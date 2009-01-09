#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "coord_query_statement.h"

#include <mysql.h>

using namespace std;

void Coord_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["lat"] = "";
  attributes["lon"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  double lat_d(atof(attributes["lat"].c_str()));
  if ((lat_d < -90.0) || (lat_d > 90.0) || (attributes["lat"] == ""))
  {
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"lat\" of the element \"coord-query\""
	<<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(Error(temp.str(), current_line_number()));
  }
  double lon_d(atof(attributes["lon"].c_str()));
  if ((lon_d < -180.0) || (lat_d > 180.0) || (attributes["lon"] == ""))
  {
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"lon\" of the element \"coord-query\""
	<<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(Error(temp.str(), current_line_number()));
  }
  lat = (int)floor(lat_d * 10000000);
  lon = (int)floor(lon_d * 10000000);
}

void Coord_Query_Statement::add_statement(Statement* statement)
{
  substatement_error(get_name(), statement);
}

void Coord_Query_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  ostringstream temp;
  temp<<"select id, min_lat, min_lon, max_lat, max_lon from area_segments "
      <<"where lat_idx = "<<calc_idx(lat)<<' '
      <<"and min_lon > "<<lon - 100000<<' '
      <<"and min_lon <= "<<lon;
  
  set< Area > areas;
  maps[output] = Set(set< Node >(), set< Way >(), set< Relation >(),
		     multiArea_query(mysql, temp.str(), lat, lon, areas));
}
