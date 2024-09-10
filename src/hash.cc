#include <istream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <array>
#include <unordered_map>
#include <stdexcept>

#include <cassert>

#include <openssl/evp.h>
#include <openssl/err.h>

#include "./global.h"
#include "./hash.h"

namespace hash {
	const std::string MD5 = "md5";
	const std::string SHA1 = "sha1";
	const std::string SHA224 = "sha224";
	const std::string SHA256 = "sha256";
	const std::string SHA384 = "sha384";
	const std::string SHA512 = "sha512";
	const std::string SHA512_224 = "sha512_224";
	const std::string SHA512_256 = "sha512_256";
	const std::string SHA3_224 = "sha3_224";
	const std::string SHA3_256 = "sha3_256";
	const std::string SHA3_384 = "sha3_384";
	const std::string SHA3_512 = "sha3_512";
} // namespace hash

namespace {
const std::array<std::string, 12> hash_algo_list{
	hash::MD5,
	hash::SHA1,
	hash::SHA224,
	hash::SHA256,
	hash::SHA384,
	hash::SHA512,
	hash::SHA512_224,
	hash::SHA512_256,
	hash::SHA3_224,
	hash::SHA3_256,
	hash::SHA3_384,
	hash::SHA3_512
};

const std::unordered_map<std::string, std::string> hash_algo_openssl_map{
	{hash::MD5, "md5"},
	{hash::SHA1, "sha1"},
	{hash::SHA224, "sha224"},
	{hash::SHA256, "sha256"},
	{hash::SHA384, "sha384"},
	{hash::SHA512, "rsa-sha512"},
	{hash::SHA512_224, "sha512-224"},
	{hash::SHA512_256, "sha512-256"},
	{hash::SHA3_224, "rsa-sha3-224"},
	{hash::SHA3_256, "rsa-sha3-256"},
	{hash::SHA3_384, "rsa-sha3-384"},
	{hash::SHA3_512, "rsa-sha3-512"},
};

const std::streamsize BUF_SIZE = 65536;

hash_res get_hash(std::istream&, const std::string&);
} // namespace

void hash_init(void) {
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
}

void hash_cleanup(void) {
	ERR_free_strings();
	EVP_cleanup();
}

// $ openssl list -digest-algorithms
std::string get_openssl_evp_name(const std::string& hash_algo) {
	try {
		return hash_algo_openssl_map.at(hash_algo);
	} catch (const std::out_of_range& e) {
		return "";
	}
}

const void* new_hash(const std::string& hash_algo) {
	return EVP_get_digestbyname(get_openssl_evp_name(hash_algo).c_str());
}

std::vector<std::string> get_available_hash_algo(void) {
	std::vector<std::string> ret;
	for (const auto& s : hash_algo_list)
		if (new_hash(s))
			ret.push_back(s);
		else if (opt::verbose || opt::debug)
			ret.push_back("*" + s);
	return ret;
}

hash_res get_file_hash(const std::string& f, const std::string& hash_algo) {
	std::ifstream ifs;
	ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	ifs.open(f, std::ifstream::binary);
	return get_hash(ifs, hash_algo);
}

hash_res get_byte_hash(const std::vector<char>& s,
	const std::string& hash_algo) {
	std::istringstream iss(std::string(s.begin(), s.end()));
	return get_hash(iss, hash_algo);
}

hash_res get_string_hash(const std::string& s, const std::string& hash_algo) {
	std::istringstream iss(s);
	return get_hash(iss, hash_algo);
}

namespace {
void openssl_evp_error(unsigned long error) {
	char buf[1024];
	ERR_error_string(error, buf);
	std::ostringstream ss;
	ss << "openssl: " << buf;
	throw std::runtime_error(ss.str());
}

hash_res get_hash(std::istream& is, const std::string& hash_algo) {
	const auto* h = reinterpret_cast<const EVP_MD*>(new_hash(hash_algo));
	assert(h);

	auto* ctx = EVP_MD_CTX_new();
	assert(ctx);
	EVP_MD_CTX_init(ctx);
	if (EVP_DigestInit_ex(ctx, h, NULL) == 0)
		openssl_evp_error(ERR_get_error());

	std::vector<char> buf(BUF_SIZE, 0);
	auto* p = &buf[0];
	auto* u = reinterpret_cast<unsigned char*>(p);
	unsigned long written = 0;

	while (1) {
		is.readsome(p, BUF_SIZE); // read sets failbit on EOF
		auto siz = is.gcount();
		if (siz == 0)
			break;
		written += static_cast<unsigned long>(siz);
		if (EVP_DigestUpdate(ctx, u, siz) == 0)
			openssl_evp_error(ERR_get_error());
	}

	unsigned int n;
	if (EVP_DigestFinal_ex(ctx, u, &n) == 0)
		openssl_evp_error(ERR_get_error());
	EVP_MD_CTX_free(ctx);

	buf.resize(n);
	return {buf, written};
}
} // namespace

std::string get_hex_sum(const std::vector<char>& sum) {
	std::ostringstream ss;
	ss << std::hex;
	for (const auto& x : sum)
		ss << std::setw(2) << std::setfill('0')
			<< static_cast<int>(x & 0xff);
	return ss.str();
}

#ifdef CONFIG_CPPUNIT
#include <cppunit/TestAssert.h>

#include "./cppunit.h"

void HashTest::test_hash_algo_openssl_map(void) {
	for (const auto& s : hash_algo_list)
		try {
			CPPUNIT_ASSERT_MESSAGE(s.c_str(),
				!hash_algo_openssl_map.at(s).empty());
		} catch (const std::out_of_range& e) {
			CPPUNIT_FAIL(s.c_str());
		}
	try {
		hash_algo_openssl_map.at("invalid");
		CPPUNIT_FAIL("");
	} catch (const std::out_of_range& e) {
	}
}

void HashTest::test_get_openssl_evp_name(void) {
	for (const auto& s : hash_algo_list)
		CPPUNIT_ASSERT_MESSAGE(s.c_str(),
			!get_openssl_evp_name(s).empty());
	CPPUNIT_ASSERT_EQUAL(get_openssl_evp_name("invalid"), std::string(""));
}

void HashTest::test_new_hash(void) {
	for (const auto& s : hash_algo_list)
		CPPUNIT_ASSERT_MESSAGE(s.c_str(), new_hash(s));
	CPPUNIT_ASSERT_EQUAL(new_hash("invalid"),
		reinterpret_cast<const void*>(NULL));
}

void HashTest::test_get_byte_hash(void) {
	const std::vector<std::tuple<std::string, std::string>> alg_sum_list_1{
		{hash::MD5, "d41d8cd98f00b204e9800998ecf8427e"},
		{hash::SHA1, "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
		{hash::SHA224, "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"},
		{hash::SHA256, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
		{hash::SHA384, "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"},
		{hash::SHA512, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"},
	};
	for (const auto& x : alg_sum_list_1) {
		const auto [hash_algo, hash_str] = x;
		const auto [b, _ignore] = get_byte_hash(std::vector<char>{},
			hash_algo);
		CPPUNIT_ASSERT_EQUAL_MESSAGE(hash_algo, get_hex_sum(b),
			hash_str);
	}

	const std::vector<std::tuple<std::string, std::string>> alg_sum_list_2{
		{hash::MD5, "48fcdb8b87ce8ef779774199a856091d"},
		{hash::SHA1, "065e431442d313aa4c4345f1c7f3d3a84a9b201f"},
		{hash::SHA224, "62f2929306a761f06a3b055aac36ec38df8e275a8b66e68c52f030d3"},
		{hash::SHA256, "e23c0cda5bcdecddec446b54439995c7260c8cdcf2953eec9f5cdb6948e5898d"},
		{hash::SHA384, "3a52aaed14b5b6f9f7208914e5c34f0e16e70a285c37fd964ab918980a40acb52be0a71d43cdabb702aa2d025ce9ab7b"},
		{hash::SHA512, "990fed5cd10a549977ef6c9e58019a467f6c7aadffb9a6d22b2d060e6989a06d5beb473ebc217f3d553e16bf482efdc4dd91870e7943723fdc387c2e9fa3a4b8"},
	};
	std::string s(1000000, 'A');
	std::vector<char> v(s.begin(), s.end());
	for (const auto& x : alg_sum_list_2) {
		const auto [hash_algo, hash_str] = x;
		const auto [b, _ignore] = get_byte_hash(v, hash_algo);
		CPPUNIT_ASSERT_EQUAL_MESSAGE(hash_algo, get_hex_sum(b),
			hash_str);
	}
}

void HashTest::test_get_string_hash(void) {
	const std::vector<std::tuple<std::string, std::string>> alg_sum_list_1{
		{hash::MD5, "d41d8cd98f00b204e9800998ecf8427e"},
		{hash::SHA1, "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
		{hash::SHA224, "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"},
		{hash::SHA256, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
		{hash::SHA384, "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"},
		{hash::SHA512, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"},
	};
	for (const auto& x : alg_sum_list_1) {
		const auto [hash_algo, hash_str] = x;
		const auto [b, _ignore] = get_string_hash("", hash_algo);
		CPPUNIT_ASSERT_EQUAL_MESSAGE(hash_algo, get_hex_sum(b),
			hash_str);
	}

	const std::vector<std::tuple<std::string, std::string>> alg_sum_list_2{
		{hash::MD5, "48fcdb8b87ce8ef779774199a856091d"},
		{hash::SHA1, "065e431442d313aa4c4345f1c7f3d3a84a9b201f"},
		{hash::SHA224, "62f2929306a761f06a3b055aac36ec38df8e275a8b66e68c52f030d3"},
		{hash::SHA256, "e23c0cda5bcdecddec446b54439995c7260c8cdcf2953eec9f5cdb6948e5898d"},
		{hash::SHA384, "3a52aaed14b5b6f9f7208914e5c34f0e16e70a285c37fd964ab918980a40acb52be0a71d43cdabb702aa2d025ce9ab7b"},
		{hash::SHA512, "990fed5cd10a549977ef6c9e58019a467f6c7aadffb9a6d22b2d060e6989a06d5beb473ebc217f3d553e16bf482efdc4dd91870e7943723fdc387c2e9fa3a4b8"},
	};
	std::string s(1000000, 'A');
	for (const auto& x : alg_sum_list_2) {
		const auto [hash_algo, hash_str] = x;
		const auto [b, _ignore] = get_string_hash(s, hash_algo);
		CPPUNIT_ASSERT_EQUAL_MESSAGE(hash_algo, get_hex_sum(b),
			hash_str);
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION(HashTest);
#endif
