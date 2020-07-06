#include <map>
#include <string>
#include <vector>

class Json
{
  public:

    Json()
    {

    }

    Json(const std::string&)
    {
      iString = s;
      Parse();
    }

    ~Json()
    {

    }

    bool IsOk()
    {

    }

    std::string Stringify(void)
    {
      std::string js;

      js += "{";

      for (const auto &element : iMap)
      {
        if (js.length() > 1)
        {
          js += ", ";
        }

        js += "\"" + element.first + "\"" + ": ";
    
        if ((element.second)[0] != '[') 
        {
          js += "\"" + element.second + "\"";
        }
        else
        {
          js += element.second;
        }
      }

      js += "}";

      return js;
    }
    
    static std::string JsonListToArray(std::vector<Json>& list)
    {
      std::string value = "[";

      for(auto& j : list)
      {
        value += j.Stringify();
        value += ",";
      }
  
      value = trim(value, ",");
    
      value += "]";

      return value;
    }

    void SetKey(const std::string& key, const std::string& value)
    {
      iMap[key] = value;
    }
    
    void Parse(void)
    {

    }
    
    void Dump(void)
    {

    }

  private:

    std::string iJsonString;

    std::map<std::string, std::string> iMap;

};