#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

# include <string>
# include <vector>
# include <set>

class LocationConfig {
	public:
		std::string					_path;
		std::string					_root;
		std::string					_index;
		bool						_autoindex;
		std::set<std::string>		_allowedMethods;
		std::string					_uploadStore;
		std::string					_redirectCode;
		std::string					_redirectUrl;
		size_t						_clientMaxBodySize;
		std::vector<std::string>	_cgiExtensions;
		std::vector<std::string>	_cgiInterpreters;

		LocationConfig();
		LocationConfig(const LocationConfig &source);
		LocationConfig &operator=(const LocationConfig &rhs);
		~LocationConfig();

		bool isCgiEnabled() const;

		// Getters
		const std::string& getPath() const;
		const std::string& getRoot() const;
		const std::string& getIndex() const;
		bool getAutoindex() const;
		const std::set<std::string>& getAllowedMethods() const;
		const std::string& getUploadStore() const;
		const std::string& getRedirectCode() const;
		const std::string& getRedirectUrl() const;
		size_t getClientMaxBodySize() const;
		const std::vector<std::string>& getCgiExtensions() const;
		const std::vector<std::string>& getCgiInterpreters() const;
		std::string getCgiPath() const;
};

#endif
