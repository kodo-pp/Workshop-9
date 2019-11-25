#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_set>

std::vector<std::vector<std::string>> worldCities ()
{
    std::ifstream in ("worldcities.csv");
    std::string line;

    std::vector<std::vector<std::string>> result;

    std::getline (in, line);
    while (!in.eof() && in.good ())
    {
      std::getline (in, line);

      std::vector<std::string> tokens;
      std::stringstream ss(line);
      std::string token;
      while (!ss.eof () && ss.good ())
      {
          std::getline (ss, token, ',');
          tokens.push_back (token);
      }

      if (tokens.size () > 1)
        result.push_back (tokens);
    }
    return result;
}

size_t howManyCountries (const std::vector<std::vector<std::string>> &world_cities)
{
    std::unordered_set<std::string> countries;
    for (const std::vector<std::string> &line : world_cities)
    {
        countries.insert (line[4]);
    }
    return countries.size ();
}


int main ()
{
  std::vector<std::vector<std::string>> world_cities = worldCities ();
  size_t number_of_countries = howManyCountries (world_cities);
  std::cout << number_of_countries << std::endl;
  return 0;
}
