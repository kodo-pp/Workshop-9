#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>


/*******************************************************************************
 *
 * Workshop 9
 *
 * 1) Download the dataset (https://simplemaps.com/data/world-cities), unzip and use it (.csv) in current workshop.
 *
 * 2) Create a vector<vector<string>> worldCities (vector of lines) where you will store the data.
 * Use worldCities for next tasks.
 *
 * 3) Create a function called howManyCountries. You should count countries and print them out.
 *
 * 4) Create a function which prints countries and its capital.
 *
 * 5) Create a function called uninhabitedLongitude. You should find longitudes (rounding to ceil, 20.7458 -> 20) where
 * there are no cities and print this longitudes out.
 *
 * 6) Find and save to file "population.txt" population by countries and cities. (use map<string,map<string,int>>).
 * If there are no info in population field use "?"-sign. Table should be looks like:
 * Russia: N_1
 * -> Moscow : N_1_1 / N_1
 * -> Saint-Petersburg : N_1_2 / N_1
 * -> ...
 *
 * USA: N_M
 * -> Chicago : N_2_1 / N_2
 * -> NY : N_2_2 / N_2
 * ->
 * ...
 *
 * where N_i -- population of considered country (if not filled use "?"),
 * N_i_i -- population of i-city (if not filled use "?"),
 * N_i_i / N_i  population of the city relative to the country ("?" if some field is not filled)
 */


/**
 * Emulates std::optional from C++17
 */
template <typename T>
class Optional
{
public:
    Optional():
        _value{},
        _present(false)
    { }

    Optional(const T& value):
        _value(value),
        _present(true)
    { }

    bool has_value() const noexcept
    {
        return _present;
    }

    T& operator*()
    {
        if (!has_value())
        {
            throw std::runtime_error("Attempt to dereference an Optional with no value");
        }
        return _value;
    }

    const T& operator*() const
    {
        if (!has_value())
        {
            throw std::runtime_error("Attempt to dereference an Optional with no value");
        }
        return _value;
    }

private:
    T _value;
    bool _present;
};


// Split the string, but don't break inside quotes
namespace
{
    size_t findIgnoringQuotedValues(const std::string& s, char delimiter, size_t offset = 0)
    {
        bool quotedFlag = false;
        while (true)
        {
            if (offset >= s.size())
            {
                return s.npos;
            }

            if (s[offset] == '"')
            {
                quotedFlag = !quotedFlag;
            }
            else if (s[offset] == ',' && !quotedFlag)
            {
                return offset;
            }

            ++offset;
        }
    }

    void recursiveSplit(
        const std::string& s,
        char delimiter,
        std::vector<std::string>& result,
        size_t offset
    )
    {
        size_t index = findIgnoringQuotedValues(s, delimiter, offset);
        if (index == s.npos)
        {
            result.emplace_back(s.substr(offset));
            return;
        }

        result.emplace_back(s.substr(offset, index - offset));
        recursiveSplit(s, delimiter, result, index + 1);
    }
}


std::vector<std::string> split(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    recursiveSplit(str, delimiter, result, 0);
    return result;
}


// Remove quotes from the string if there are any
std::string maybeUnquote(const std::string& str)
{
    size_t left = (str[0] == '"') ? 1 : 0;
    size_t right = (str[str.size() - 1] == '"') ? str.size() - 1 : str.size();
    size_t len = right - left;
    return str.substr(left, len);
}


struct ParsedArguments
{
    std::string inputFileName;
    std::string outputFileName;
};


struct City
{
    std::string name;
    std::string countryName;
    double longitude;
    Optional<int> population;
    bool isCapital;
};


class FileReader
{
public:
    FileReader(const std::string& fileName):
        stream(fileName)
    {
        if (!stream.is_open())
        {
            throw std::runtime_error("Unable to open file " + fileName + " for reading");
        }
    }

    std::string readLine()
    {
        std::string line;
        std::getline(stream, line);

        if (stream.eof())
        {
            return "";
        }
        if (!stream.good())
        {
            throw std::runtime_error("Read error");
        }

        return line;
    }

    bool eof() const
    {
        return stream.eof();
    }

private:
    std::ifstream stream;
};


/**
 * File writer which throws exceptions if any operation fails
 */
class FileWriter
{
public:
    FileWriter(const std::string& fileName):
        stream(fileName)
    {
        if (!stream.is_open())
        {
            throw std::runtime_error("Unable to open file " + fileName + " for writing");
        }
    }

    void writeLine(const std::string& line)
    {
        stream << line << '\n';
        if (!stream.good())
        {
            throw std::runtime_error("Write error");
        }
    }

private:
    std::ofstream stream;
};


ParsedArguments parseArguments(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: population <input> <index>" << std::endl;
        std::cerr << "Arguments:" << std::endl;
        std::cerr << "    <input>       - Input file name" << std::endl;
        std::cerr << "    <output>      - Output file name (default: population.txt)" << std::endl;
        exit(1);
    }

    ParsedArguments args;
    args.inputFileName = argv[1];
    args.outputFileName = argc < 3 ? "population.txt" : argv[2];
    return args;
}


using CityList = std::vector<City>;


CityList readCsv(FileReader& reader)
{
    CityList cities;

    reader.readLine();  // Ignore header

    try
    {
        while (true)
        {
            std::string inputLine = reader.readLine();
            if (reader.eof())
            {
                break;
            }

            std::vector<std::string> tokens = split(inputLine, ',');
            for (std::string& token : tokens)
            {
                token = maybeUnquote(token);
            }

            const std::string& cityName = tokens.at(1);
            const std::string& longitudeStr = tokens.at(3);
            const std::string& countryName = tokens.at(4);
            const std::string& capitalType = tokens.at(8);
            const std::string& populationStr = tokens.at(9);

            double longitude = std::stod(longitudeStr);
            bool isCapital = (capitalType == "primary");
            Optional<int> population =
                populationStr.empty()
                ? Optional<int>{}
                : std::stoi(populationStr);

            cities.push_back(City{cityName, countryName, longitude, population, isCapital});
        }
    }
    catch (const std::out_of_range&)
    {
        throw std::runtime_error("The CSV file seems to be broken or to have invalid format");
    }

    return cities;
}


using CountryList = std::vector<std::pair<std::string, Optional<City>>>;


CountryList listCountries(const CityList& worldCities)
{
    std::map<std::string, Optional<City>> countries;

    for (const City& city : worldCities)
    {
        const std::string& countryName = city.countryName;

        countries[countryName];  // Creates a default value, this line cannot be omitted safely

        if (city.isCapital)
        {
            countries.at(countryName) = city;
        }
    }

    return CountryList(countries.begin(), countries.end());
}


std::set<int> findUninhabitableLongitudes(const CityList& worldCities)
{
    const int longitudeValuesRange = 180;
    std::vector<int> range(2 * longitudeValuesRange - 1); // [-180; 0) U [0; 180]
    std::iota(range.begin(), range.end(), -longitudeValuesRange);
    std::set<int> longitudes(range.begin(), range.end());

    for (const City& city : worldCities)
    {
        int lng = round(city.longitude);
        longitudes.erase(lng);
    }

    return longitudes;
}


void printCountriesInfo(const CityList& worldCities)
{
    CountryList countries = listCountries(worldCities);

    std::cout << "Number of countries: " << countries.size() << std::endl;
    std::cout << std::endl;

    std::cout << "Country capitals: " << std::endl;
    for (const std::pair<std::string, Optional<City>>& country : countries)
    {
        std::string countryName;
        Optional<City> capital;
        std::tie(countryName, capital) = country;

        std::cout
            << "    "
            << countryName
            << ": "
            << (capital.has_value() ? (*capital).name : std::string("?"))
            << std::endl;
    }
    std::cout << std::endl;
}


void printUninhabitableLongitudes(const CityList& worldCities)
{
    std::cout << "Uninhabitable longitudes: ";
    std::set<int> uninhabitableLongitudes = findUninhabitableLongitudes(worldCities);
    for (int lng : uninhabitableLongitudes)
    {
        std::cout << lng << ' ';
    }
    std::cout << std::endl << std::endl;
}


// city name -> city population [if specified]
using CitiesPopulation = std::map<std::string, Optional<int>>;

// country name -> (country population, CitiesPopulation)
using CountriesPopulation = std::map<std::string, std::pair<long long, CitiesPopulation>>;


CountriesPopulation summarizePopulationInfo(const CityList& worldCities)
{
    CountriesPopulation countriesPopulation;
    for (const City& city : worldCities)
    {
        if (city.population.has_value())
        {
            countriesPopulation[city.countryName].first += *city.population;
        }
        countriesPopulation[city.countryName].second.emplace(city.name, city.population);
    }

    return countriesPopulation;
}


void writePopulationSummary(const CityList& worldCities, std::string outputFileName)
{
    FileWriter writer(outputFileName);
    CountriesPopulation countriesPopulation = summarizePopulationInfo(worldCities);

    using CountryPair = std::pair<std::string, std::pair<long long, CitiesPopulation>>;
    std::vector<CountryPair> countriesPopulationList(countriesPopulation.begin(), countriesPopulation.end());

    // Sort by total population but preserve original lexicographic order in case of equal populations
    std::stable_sort(
        countriesPopulationList.rbegin(),
        countriesPopulationList.rend(),
        [](const CountryPair& a, const CountryPair& b) {
            return a.second.first < b.second.first;
        }
    );

    for (const CountryPair& countryPair : countriesPopulationList)
    {
        const std::string& countryName = countryPair.first;
        const CitiesPopulation& citiesPopulation = countryPair.second.second;
        long long totalPopulation = countryPair.second.first;
        writer.writeLine(countryName + ": " + std::to_string(totalPopulation));

        using CityPair = std::pair<std::string, Optional<int>>;
        std::vector<CityPair> citiesPopulationList(citiesPopulation.begin(), citiesPopulation.end());

        std::stable_sort(
            citiesPopulationList.rbegin(),
            citiesPopulationList.rend(),
            [](const CityPair& a, const CityPair& b) {
                long long valueA = a.second.has_value() ? (*a.second + 1) : 0;
                long long valueB = b.second.has_value() ? (*b.second + 1) : 0;
                return valueA < valueB;
            }
        );

        for (const CityPair& cityPair : citiesPopulationList)
        {
            const std::string& cityName = cityPair.first;
            Optional<int> population = cityPair.second;

            if (population.has_value())
            {
                double relativePopulation = static_cast<double>(*population) / totalPopulation;
                writer.writeLine("-> " + cityName + " : " + std::to_string(relativePopulation));
            }
        }

    }
    std::cout << "Population data written to " << outputFileName << std::endl;
}


int main(int argc, char** argv)
{
    try
    {
        ParsedArguments args = parseArguments(argc, const_cast<const char**>(argv));

        FileReader reader(args.inputFileName);
        CityList worldCities = readCsv(reader);

        printCountriesInfo(worldCities);
        printUninhabitableLongitudes(worldCities);

        writePopulationSummary(worldCities, args.outputFileName);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
