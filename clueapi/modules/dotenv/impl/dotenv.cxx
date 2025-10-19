/**
 * @file dotenv.cxx
 *
 * @brief Implementation of the dotenv module.
 */

#include "clueapi/modules/dotenv/dotenv.hxx"

#include <fstream>
#include <iterator>

#include <boost/algorithm/string/trim.hpp>

#ifdef CLUEAPI_USE_DOTENV_MODULE
namespace clueapi::modules::dotenv {
    void c_dotenv::load(std::string filename, bool trim_values) {
        m_filename = std::move(filename);

        m_trim_values = trim_values;

        if (m_filename.empty())
            return;

        parse();
    }

    void c_dotenv::destroy() {
        m_container.destroy();

        m_filename.clear();
    }

    void c_dotenv::parse() {
        std::ifstream file{m_filename};

        if (!file.is_open())
            return;

        {
            auto lines_count = std::count(
                std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');

            if (lines_count)
                m_container.reserve(lines_count);

            file.clear();
            file.seekg(0, std::ios::beg);
        }

        std::string line{};

        line.reserve(512);

        while (std::getline(file, line)) {
            auto first_char_pos = line.find_first_not_of(" \t");

            if (first_char_pos == std::string::npos || line[first_char_pos] == '#') {
                continue;
            }

            auto pos = line.find('=');

            if (pos == std::string::npos)
                continue;

            auto key = boost::trim_copy(line.substr(0, pos));

            if (key.empty())
                continue;

            auto value = line.substr(pos + 1);

            if (m_trim_values)
                value = boost::trim_copy(value);

            m_container.set(ENV_NAME_RT(key), value);
        }
    }
} // namespace clueapi::modules::dotenv
#endif // CLUEAPI_USE_DOTENV_MODULE