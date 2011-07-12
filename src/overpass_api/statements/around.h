#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Around_Statement : public Statement
{
  public:
    Around_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "around"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Around_Statement() {}

    string get_source_name() const { return input; }

    set< pair< Uint32_Index, Uint32_Index > > calc_ranges
        (const map< Uint32_Index, vector< Node_Skeleton > >& input_nodes);
    
    bool is_inside(double lat, double lon) const;

  private:
    string input, output;
    double radius;
    vector< pair< double, double > > lat_lons;
};

#endif