//
// Created by brany on 28.09.23.
//

#ifndef FLEXUS_STATSTREAMPARSER_H
#define FLEXUS_STATSTREAMPARSER_H


#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <ostream>
#include <cassert>
#include <iterator>
#include <iostream>

class StatStreamHelper {

private:
  // The map populate by the parser, and used to return value and key
  std::map<std::string, int> result_map{};

  // Character defined as whitespace for the triming operation
  const std::string WHITESPACE = " \n\r\t\f\v";


  /**
   * @brief Trim whitespace between the start and the first non-space char. of a string
   * @param s a string reference
   * @return A copy of a string without whitespace at the beginning
   */
  std::string ltrim(const std::string &s)
  {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
  }

  /**
   * @brief Trim whitespace between last non-space char. and the end of a string
   * @param s a string reference
   * @return A copy of a string without whitespace at the end
   */
  std::string rtrim(const std::string &s)
  {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
  }

  /**
   * @brief Helper function that trim space the both tail-end of a string
  * @param s a string reference
  * @return A copy of a string without whitespace at the beginning and the end
   */
  std::string trim(const std::string &s)
  {
    return rtrim(ltrim(s));
  }

/**
 * @brief Convert a stringstream from a StatManager object to a functional map.
 *         The parsing use the EOL UNIX code (= \n) to split the line,
 *         the entry are separate from their value using the whitespace char
 *         A typicall output from the stream looks like this:
 *
 *         all
 *            sys-FastCacheTest config-Hits:Evict:Exclusive 0
 *            sys-FastCacheTest config-Hits:Evict:Migratory 0
 *            sys-FastCacheTest config-Hits:Evict:Modified 0
 *            sys-FastCacheTest config-Hits:Evict:Owned 0
 *            sys-FastCacheTest config-Hits:Evict:Shared 0
 *            sys-FastCacheTest config-Hits:EvictD:Exclusive 0
 *            sys-FastCacheTest config-Hits:EvictD:Migratory 0
 *            sys-FastCacheTest config-Hits:EvictD:Modified 0
 *
* @param stats_stream A stringstream from a StatManager object
 */
  void stream_to_map(std::stringstream& stats_stream)
  {
    std::string  word;
    std::vector<std::string> ss_as_list;

    // Split the stream evey EOL and put them into a list
    while (std::getline(stats_stream, word, '\n')) {
      ss_as_list.push_back(word);
    }

    // Erase the first element of the list, which represent the ?statstype? the entry are from
    ss_as_list.erase(ss_as_list.begin());

    // Iterrate over the line the split them by their whitespace character, and populate the map
    for (auto it = ss_as_list.begin(); it != ss_as_list.end(); it++)
    {
      std::vector<std::string> tokens;
      std::stringstream line(trim(*it)); // Trim the front and back whitespace of the line

      // Split by ONE (1) exact whitespace, additional whitespace would result into more splits
      // TODO: whitespace agnostic split
      while (std::getline(line, word, ' '))
      {
        tokens.push_back(word);
      }

      // Placeholder for the map value
      int entry_value{0};

      /**
       * To avoid casting an actual a string of litteral character instead of a number.
       * This try/catch is designed to skip the loop (thus the actual line if the value cannot be casted to <int>)
       */
      try {
        entry_value = std::stoi(tokens[2]);
      } catch (const std::exception&) {
        continue;
      }

      /**
       * @brief From this point ON, it is asserted that the tokens list is no more than 3 token long.
       * Because the following cannot deal with any other possibility
       */
      assert((tokens.size() == 3));

      // Concatenate 1st and 2nd element to represent the full key made of ('component name' 'entry')
      std::string key = tokens[0] + " " + tokens[1];
      result_map[key] = entry_value;
    }
  }

public:
  /**
   * @brief Helper constructor.
    * @param stats_stream A stringstream from a StatManager object
   */
  StatStreamHelper(std::stringstream&  stats_stream)  {
    //? Test and check stats_stream and other things if need
    stream_to_map(stats_stream);
  }

  /**
   * @brief Return the value of a map entry
   * @param key Existing key of the map
   * @param value_placeholder A reference, to place the resulting map access' result to
   * @return A boolean indicating if the operation was sucessfull
   */
  bool get_entry_by_name(const std::string& key, int& value_placeholder)
  {
    // it == result_map.end( if key does not exist
    auto it = result_map.find(key);

    if (it != result_map.end())
    {
      value_placeholder = it->second;
      return true;
    }
    else
    {
      return false;
    }
  }

  /**
   * @brief A getter to return a copy of the in-memory map data
   * @return the results map
   */
  std::map<std::string, int> get_result_map()
  {
    return result_map;
  }

};

#endif // FLEXUS_STATSTREAMPARSER_H
