#pragma once
#include"const.h"
struct SectionInfo {
	SectionInfo() {}
	~SectionInfo() {
		_section_datas.clear();
	}

	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}

	SectionInfo& operator = (const SectionInfo& src) {
		if (&src == this) {
			return *this;
		}

		this->_section_datas = src._section_datas;
		return *this;
	}

	std::map<std::string, std::string> _section_datas;
	std::string  operator[](const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		// 这里可以添加一些边界检查  
		return _section_datas[key];
	}

	std::string GetValue(const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		// 这里可以添加一些边界检查  
		return _section_datas[key];
	}
};
class ConfigMgr
{

public:
	~ConfigMgr() {
		_config_map.clear();
	}
	SectionInfo operator[](const std::string& section) {
		if (_config_map.find(section) != _config_map.end()) {
			return _config_map[section];
		}
		return SectionInfo();
	}
	static ConfigMgr& Inst() {
		static ConfigMgr cfg_mgr;
		return cfg_mgr;
	}
	ConfigMgr(const ConfigMgr&src);
	ConfigMgr& operator=(const ConfigMgr& src) {
		if(&src==this){
			return *this;
		}
		_config_map = src._config_map;
	}
	std::string GetValue(const std::string& section, const std::string& key);
private:
	ConfigMgr();
	std::map<std::string, SectionInfo>_config_map;
};
/*通过嵌套映射（std::map<std::string, SectionInfo>）完美对应了 INI 文件中的 [Section] -> 
Key = Value 结构。这种将文件结构直接映射为内存数据结构的做法，极大提高了配置读取的效率。
运算符重载的便利性：你在 ConfigMgr 和 SectionInfo 中都重载了 operator[]。这让你可以像操作原生字典一样访问配置*/
