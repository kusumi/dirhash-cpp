#ifndef SRC_HASH_H_
#define SRC_HASH_H_

#include <vector>
#include <tuple>
#include <string>

typedef std::tuple<std::vector<char>, unsigned long> hash_res;

namespace hash {
	extern const std::string MD5;
	extern const std::string SHA1;
	extern const std::string SHA224;
	extern const std::string SHA256;
	extern const std::string SHA384;
	extern const std::string SHA512;
	extern const std::string SHA512_224;
	extern const std::string SHA512_256;
	extern const std::string SHA3_224;
	extern const std::string SHA3_256;
	extern const std::string SHA3_384;
	extern const std::string SHA3_512;
} // namespace hash

void hash_init(void);
void hash_cleanup(void);
std::string get_openssl_evp_name(const std::string&);
const void* new_hash(const std::string&);
std::vector<std::string> get_available_hash_algo(void);
hash_res get_file_hash(const std::string&, const std::string&);
hash_res get_byte_hash(const std::vector<char>&, const std::string&);
hash_res get_string_hash(const std::string&, const std::string&);
std::string get_hex_sum(const std::vector<char>&);

#ifdef CONFIG_CPPUNIT
#include <cppunit/TestFixture.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>

class HashTest: public CPPUNIT_NS::TestFixture {
	public:
	CPPUNIT_TEST_SUITE(HashTest);
	CPPUNIT_TEST(test_hash_algo_openssl_map);
	CPPUNIT_TEST(test_get_openssl_evp_name);
	CPPUNIT_TEST(test_new_hash);
	CPPUNIT_TEST(test_get_byte_hash);
	CPPUNIT_TEST(test_get_string_hash);
	CPPUNIT_TEST_SUITE_END();

	private:
	void test_hash_algo_openssl_map(void);
	void test_get_openssl_evp_name(void);
	void test_new_hash(void);
	void test_get_byte_hash(void);
	void test_get_string_hash(void);
};
#endif
#endif // SRC_HASH_H_
