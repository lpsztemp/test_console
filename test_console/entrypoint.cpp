#include <camaas/idatastorage.h>
#include <chsvlib/chsvmath.h>
#include <string>
#include <iostream>
#include <iomanip>

using namespace CAMaaS;

std::string operator""_s(const char* psz, std::size_t cch) {return std::string(psz, cch);}

void TestContiguousDataSource(OutputBinaryDataContiguousAccessRef output_side, InputBinaryDataContiguousAccessRef input_side)
{
	auto osWrite = output_side.write();
	osWrite.write(std::vector<std::uint8_t>({0x30, 0x31, 0x32, 0x33, 0x34}));
	osWrite.write(std::vector<std::uint8_t>({0x35, 0x36, 0x37, 0x38, 0x39}));
	auto osWrite2 = output_side.write(20);
	osWrite2.write(std::vector<std::uint8_t>({'a', 'b', 'c', 'd', 'e'}));
	osWrite2.write(std::vector<std::uint8_t>({'e', 'f', 'g', 'h', 'i'}));
	output_side.delete_block(10, 10);
	auto osRead1 = input_side.read();
	auto v1 = osRead1.read_as<std::vector<std::uint8_t>>(10);
	auto vout10 = osRead1.read_as<std::vector<std::uint8_t>>(5);
	auto osRead2 = input_side.read(15);
	auto vout11 = osRead2.read_as<std::vector<std::uint8_t>>(std::size_t(input_side.size() - 15));
	std::move(vout10.begin(), vout10.end(), std::back_inserter(v1));
	std::move(vout11.begin(), vout11.end(), std::back_inserter(v1));
	auto v2 = input_side.read().read_all_as<std::vector<std::uint8_t>>();
	std::cout << "Vectors are equal: " << std::boolalpha << (v1 == v2) << "\n";
	std::cout << "Read contents: ";
	for (auto b:v2)
		std::cout << std::hex << b;
	std::cout << "\n";
}

constexpr std::uint8_t operator""_b(unsigned long long int val)
{
	return std::uint8_t(val);
}

template <class _V>
constexpr std::vector<_V> make_vector(std::initializer_list<_V> lst)
{
	return std::vector<_V>(lst);
}

using namespace CAMaaS;

bool copy(IInputByteStream* pFrom, IOutputByteStream* pTo)
{
   const std::size_t WINDOW_SIZE = 0x1000;
   std::uint8_t buf[WINDOW_SIZE];
   std::uint64_t cbRead = 1;
   while(cbRead != 0)
   {
      cbRead = WINDOW_SIZE;
      std::int32_t nError = pFrom->Read(buf, std::addressof(cbRead));
      if (nError < 0)
         return false; //error
      nError = pTo->Write(buf, cbRead);
      if (nError < 0)
         return false; //error
   }
   return true;
}

template <class...Args>
static auto deduce_iterator_range(Args&&...args) -> 
	typename std::enable_if<
		_Implementation::is_byte_type<
			typename std::iterator_traits<
				typename Chusov::identity<
					decltype(_Implementation::deduce_iterator_range(std::forward<Args>(args)...))
				>::type::iterator
			>::value_type
		>::value, 
		decltype(_Implementation::deduce_iterator_range(std::forward<Args>(args)...))
	>::type
{
	return _Implementation::deduce_iterator_range(std::forward<Args>(args)...);
}
template <class...Args>
static auto _is_deducable_to_range(int, decltype(deduce_iterator_range(std::declval<Args>()...))* = nullptr) -> std::true_type;
template <class...Args>
static auto _is_deducable_to_range(...) -> std::false_type;

template <class...Args>
struct is_deducable_to_range:decltype(_is_deducable_to_range<Args...>(int())) {};

int main(int, char**)
{
	_Implementation::is_byte_type<void>::value;
	static_assert(is_deducable_to_range<void*, std::size_t>::value, "");
	auto rng = _Implementation::deduce_iterator_range(nullptr, std::size_t());
	static_assert(is_deducable_to_range<std::uint8_t*, std::size_t>::value, "");
	static_assert(!is_deducable_to_range<_Implementation::MapBasedKeyedDataBaseImplementation::key_type>::value, "");

	auto p = _Implementation::KeyedDataBaseBridge<_Implementation::MapBasedKeyedDataBaseImplementation>();
	////auto fail = CKeyedData<_Implementation::MapBasedKeyedDataBaseImplementation>();
	//auto b1 = _Implementation::KeyedDataBaseBridge<_Implementation::MapBasedKeyedDataBaseImplementation>::is_readable_type::value;
	//auto b2 = _Implementation::KeyedDataBaseBridge<_Implementation::MapBasedKeyedDataBaseImplementation>::is_writable_type::value;
	p.Insert(std::piecewise_construct, std::forward_as_tuple("Key", 3), std::forward_as_tuple(make_binary_memory_storage_adapter(std::string("value")).get()));
	//p.Insert(_Implementation::MapBasedKeyedDataBaseImplementation::key_type("Key", 3),
	//	_Implementation::MapBasedKeyedDataBaseImplementation::value_type(make_binary_memory_storage_adapter(std::string("value")).get()));
	//auto mp = _Implementation::MapBasedKeyedDataBaseImplementation();
	//mp.insert(std::piecewise_construct, std::forward_as_tuple("Key", 3), std::forward_as_tuple(make_binary_memory_storage_adapter(std::string("value")).get()));

	/*std::cout << "===File test===\n";
	auto strFile = u8"Test file.txt"_s;
	auto file = make_file_based_data_storage<FileReadWrite, FileCreateAlways>(strFile);
	TestContiguousDataSource(file, file);
	std::cout << "===Preallocated buffer test===\n";
	std::uint8_t pBuffer[100];
	auto buff = make_inmemory_preallocated_data_storage(pBuffer, sizeof(pBuffer));
	TestContiguousDataSource(buff, buff);
	std::cout << "===Preallocated buffer test with owning===\n";
	auto alloc = std::allocator<std::uint8_t>();
	auto p2 = std::allocator_traits<std::allocator<std::uint8_t>>::allocate(alloc, sizeof(pBuffer));
	auto buff2 = make_inmemory_preallocated_data_storage(own_buffer(p2, sizeof(pBuffer), alloc));
	TestContiguousDataSource(buff2, buff2);
	std::cout << "===Reading from existing buffer===\n";
	auto pRead = make_inmemory_input_data_source(pBuffer, buff.size());
	auto vRead = pRead.read().read_all_as<std::vector<std::uint8_t>>();
	std::cout << "Read contents from the input buffer: ";
	for (auto b:vRead)
		std::cout << std::hex << b;
	std::cout << "\n";
	try
	{
		represent_as<OutputBinaryDataOwn>(pRead);
		std::cout << "Conversion did NOT cause an expected error!\n";
	}catch (Chusov::Exceptions::ChsvCodeException& ex)
	{
		std::cout << "Conversion caused expected error: " << ex.what() << "\n";
	}
	std::cout << "===Cached buffer test===\n";
	auto buff_cached = make_inmemory_data_storage();
	TestContiguousDataSource(buff_cached, buff_cached);*/

	return 0;
	
}