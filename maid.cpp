#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

static uint32_t joaat(const std::string& str)
{
	uint32_t hash = 0;
	for (const char c : str)
	{
		hash += c;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

using hash_to_string_t = std::string(*)(uint32_t);

static std::string hex(uint32_t hash)
{
	std::stringstream ss = {};
	ss << "0x" << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << hash;
	return ss.str();
}

static std::string dec_unsigned(uint32_t hash)
{
	return std::to_string(hash);
}

static std::string dec_signed(uint32_t hash)
{
	return std::to_string((int32_t)hash);
}

static void for_each_hash_to_string_algo(std::function<void(const char*, hash_to_string_t)>&& func)
{
	func("hex", &hex);
	func("dec_unsigned", &dec_unsigned);
	func("dec_signed", &dec_signed);
}

static void output_db(const std::string& name, const std::map<uint32_t, std::optional<std::string>>& database, hash_to_string_t hash_to_string)
{
	std::ofstream ofstream(std::string("out/").append(name).append(".csv"));
	ofstream << "hash,key" << std::endl;
	for (const auto& entry : database)
	{
		ofstream << hash_to_string(entry.first) << ",";
		if (entry.second.has_value())
		{
			ofstream << entry.second.value();
		}
		ofstream << std::endl;
	}
}

static void output_db(const std::string& db_name, const std::map<uint32_t, std::optional<std::string>>& database)
{
	for_each_hash_to_string_algo([&](const char* hts_name, hash_to_string_t hts_algo)
	{
		output_db(std::string(db_name).append(1, '-').append(hts_name), database, hts_algo);
	});
}

static void output_dict(const char* name, const std::map<uint32_t, std::vector<std::string>>& dictionary, hash_to_string_t hash_to_string)
{
	std::ofstream ofstream(std::string("out/dictionary-").append(name).append(".tsv"));
	ofstream << "hash\tkey(s)" << std::endl;
	for (const auto& entry : dictionary)
	{
		ofstream << hash_to_string(entry.first) << "\t";
		for (auto key_it = entry.second.begin(); key_it != entry.second.end(); ++key_it)
		{
			if (key_it != entry.second.begin())
			{
				ofstream << ", ";
			}
			ofstream << *key_it;
		}
		ofstream << std::endl;
	}
}

static void output_dict(const std::map<uint32_t, std::vector<std::string>>& dictionary)
{
	for_each_hash_to_string_algo([&](const char* hts_name, hash_to_string_t hts_algo)
	{
		output_dict(hts_name, dictionary, hts_algo);
	});
}

static void save_db(const std::string& name, const std::map<uint32_t, std::optional<std::string>>& database)
{
	std::set<std::string> lines = {};
	for (const auto& entry : database)
	{
		if (entry.second.has_value())
		{
			lines.emplace(entry.second.value());
		}
		else
		{
			lines.emplace(hex(entry.first));
		}
	}
	std::ofstream ofstream(std::string("raw/").append(name).append(".txt"));
	for (auto&& line : lines)
	{
		ofstream << line << std::endl;
	}
}

static void load_db(const std::string& name, std::map<uint32_t, std::optional<std::string>>& database)
{
	std::ifstream ifstream(std::string("raw/").append(name).append(".txt"));
	for (std::string line; std::getline(ifstream, line); )
	{
		if (line.length() > 4 && line.substr(0, 4) == "dec:")
		{
			database.emplace((uint32_t)std::stol(line.substr(4)), std::nullopt);
		}
		else if (line.length() > 2 && line.substr(0, 2) == "0x")
		{
			database.emplace(std::stoul(line.substr(2), nullptr, 16), std::nullopt);
		}
		else if (!line.empty())
		{
			std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) { return std::tolower(c); });
			uint32_t hash = joaat(line);
			auto entry = database.find(hash);
			if (entry == database.end())
			{
				database.emplace(std::move(hash), std::move(line));
			}
			else if (!entry->second.has_value())
			{
				entry->second = std::move(line);
			}
			else if (entry->second.value() != line)
			{
				std::cout << "COLLISION: " << entry->second.value() << " <-> " << line << std::endl;
			}
		}
	}
}

static std::map<std::string, std::map<uint32_t, std::optional<std::string>>> databases;

static void load_db(const std::string& name)
{
	std::map<uint32_t, std::optional<std::string>> database = {};
	load_db(name, database);
	databases.emplace(name, std::move(database));
}

static void copy_db(std::map<uint32_t, std::optional<std::string>>& target, const char* source)
{
	if(databases.find(source) == databases.end())
	{
		load_db(source);
	}
	auto database = databases.at(source);
	target.insert(database.begin(), database.end());
}

int main()
{
	try
	{
		databases = {};
		{
			std::map<uint32_t, std::optional<std::string>> automobiles_and_trailers = {};
			copy_db(automobiles_and_trailers, "automobiles");
			copy_db(automobiles_and_trailers, "trailers");
			databases.emplace("automobiles_and_trailers", std::move(automobiles_and_trailers));
		}
		{
			std::map<uint32_t, std::optional<std::string>> vehicles = {};
			copy_db(vehicles, "automobiles_and_trailers");
			copy_db(vehicles, "bikes");
			copy_db(vehicles, "boats");
			copy_db(vehicles, "helis");
			copy_db(vehicles, "planes");
			copy_db(vehicles, "submarines");
			copy_db(vehicles, "trains");
			databases.emplace("vehicles", std::move(vehicles));
		}
		{
			std::map<uint32_t, std::optional<std::string>> entities = {};
			copy_db(entities, "peds");
			copy_db(entities, "peds_crash");
			databases.emplace("peds_complete", std::move(entities));
		}
		{
			std::map<uint32_t, std::optional<std::string>> objects = {};
			load_db("objects_downtown", objects);
			databases.emplace("objects_downtown", objects);
			load_db("objects_drawable", objects);
			databases.emplace("objects_drawable", objects);
			load_db("objects", objects);
			databases.emplace("objects_complete", std::move(objects));	
		}
		{
			std::map<uint32_t, std::optional<std::string>> weapon_types = {};
			load_db("weapons", weapon_types);
			databases.emplace("weapons", weapon_types);
			load_db("weapon_types", weapon_types);
			databases.emplace("weapon_types", std::move(weapon_types));
		}
		{
			std::unordered_set<std::string> outputed_dbs = {};
			for (const auto& database : databases)
			{
				output_db(database.first, database.second);
				outputed_dbs.emplace(database.first);
			}
			{
				auto weapons = databases.at("weapons");
				auto i = databases.at("weapon_types").begin();
				while (i != databases.at("weapon_types").end())
				{
					if (weapons.find(i->first) != weapons.end())
					{
						i = databases.at("weapon_types").erase(i);
					}
					else
					{
						i++;
					}
				}
			}
			for (const auto& file : std::filesystem::directory_iterator("raw"))
			{
				auto filename = file.path().filename().u8string();
				if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".txt")
				{
					auto database_name = filename.substr(0, filename.length() - 4);
					if (databases.find(database_name) == databases.end())
					{
						load_db(database_name);
					}
					save_db(database_name, databases.at(database_name));
					if (outputed_dbs.find(database_name) == outputed_dbs.end())
					{
						output_db(database_name, databases.at(database_name));
					}
				}
			}
		}
		{
			std::map<uint32_t, std::vector<std::string>> dictionary = {};
			for (const auto& database : databases)
			{
				for (const auto& db_entry : database.second)
				{
					if (!db_entry.second.has_value())
					{
						continue;
					}
					auto dict_entry = dictionary.find(db_entry.first);
					if (dict_entry == dictionary.end())
					{
						dictionary.emplace(db_entry.first, std::vector<std::string>(1, db_entry.second.value()));
					}
					else if (std::find(dict_entry->second.begin(), dict_entry->second.end(), db_entry.second.value()) == dict_entry->second.end())
					{
						dict_entry->second.emplace_back(db_entry.second.value());
					}
				}
			}
			output_dict(dictionary);
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cout << "Good luck figuring this out!" << std::endl;
		return 1;
	}
	return 0;
}
