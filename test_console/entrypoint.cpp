#include <camaas/idatastorage.h>
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
	auto vout11 = osRead2.read_as<std::vector<std::uint8_t>>(input_side.size() - 15);
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

int main(int, char**)
{
	{
		auto alloc = AllocatorOwn<std::uint8_t>(InvokeInterfaceGetter(&CreateDefaultAllocator));
		auto p = alloc.allocate(10);
		auto data_manager = make_inmemory_preallocated_data_storage(own_buffer(p, 10));
		data_manager.write().write(make_vector({1_b, 2_b, 3_b, 4_b}));
		for (auto&&b:data_manager.GetBuffer())
			std::cout << int(b);
		std::cout << "\n";
	}
	std::cout << "===File test===\n";
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
	TestContiguousDataSource(buff_cached, buff_cached);

	return 0;
	
}