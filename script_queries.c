#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include "script_datatypes.h"
#include "script_queries.h"
#include "user_interface.h"
#include "vigilance_control.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <mysql.h>

using namespace std;

MYSQL_RES* mysql_query_wrapper(MYSQL* mysql, string query)
{
  int query_status(mysql_query(mysql, query.c_str()));
  if (query_status)
  {
    ostringstream temp;
    temp<<"Error during SQL query ";
    temp<<'('<<query_status<<"):\n";
    temp<<"Query: "<<query<<'\n';
    temp<<"Error: "<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }

  MYSQL_RES* result(mysql_store_result(mysql));
  if (!result)
  {
    if (is_timed_out())
      runtime_error("Your query timed out.", cout);
    ostringstream temp;
    temp<<"Error during SQL query (result is null pointer)\n";
    temp<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }
  
  return result;
}

MYSQL_RES* mysql_query_use_wrapper(MYSQL* mysql, string query)
{
  int query_status(mysql_query(mysql, query.c_str()));
  if (query_status)
  {
    ostringstream temp;
    temp<<"Error during SQL query ";
    temp<<'('<<query_status<<"):\n";
    temp<<"Query: "<<query<<'\n';
    temp<<"Error: "<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }

  MYSQL_RES* result(mysql_use_result(mysql));
  if (!result)
  {
    if (is_timed_out())
      runtime_error("Your query timed out.", cout);
    ostringstream temp;
    temp<<"Error during SQL query (result is null pointer)\n";
    temp<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }
  
  return result;
}

void mysql_query_null_wrapper(MYSQL* mysql, string query)
{
  int query_status(mysql_query(mysql, query.c_str()));
  if (query_status)
  {
    ostringstream temp;
    temp<<"Error during SQL query ";
    temp<<'('<<query_status<<"):\n";
    temp<<"Query: "<<query<<'\n';
    temp<<"Error: "<<mysql_error(mysql)<<'\n';
    runtime_error(temp.str(), cout);
  }
}

int int_query(MYSQL* mysql, string query)
{
  int result_val(0);
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return 0;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  if ((row) && (row[0]))
    result_val = atoi(row[0]);
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  return result_val;
}

pair< int, int > intint_query(MYSQL* mysql, string query)
{
  pair< int, int > result_val(make_pair< int, int >(0, 0));
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_val;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  if ((row) && (row[0]) && (row[1]))
    result_val = make_pair< int, int >(atoi(row[0]), atoi(row[1]));
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  return result_val;
}

set< int >& multiint_query(MYSQL* mysql, string query, set< int >& result_set)
{
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_set;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]))
  {
    result_set.insert(atoi(row[0]));
    row = mysql_fetch_row(result);
  }
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  return result_set;
}

set< Node >& multiNode_query(MYSQL* mysql, string query, set< Node >& result_set)
{
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_set;
	
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]))
  {
    result_set.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
    row = mysql_fetch_row(result);
  }
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  return result_set;
}

set< Area >& multiArea_query(MYSQL* mysql, string query, int lat, int lon, set< Area >& result_set)
{
  MYSQL_RES* result(mysql_query_use_wrapper(mysql, query));
  if (!result)
    return result_set;
	
  map< int, bool > area_cands;
  set< int > area_definitives;
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]) && (row[3]) && (row[4]))
  {
    int id(atoi(row[0]));
    int min_lon(atoi(row[2]));
    int max_lon(atoi(row[4]));
    if (max_lon > lon)
    {
      if (min_lon < lon)
      {
	int min_lat(atoi(row[1]));
	int max_lat(atoi(row[3]));
	if ((min_lat < lat) && (max_lat < lat))
	  area_cands[id] = !area_cands[id];
	else if ((min_lat < lat) || (max_lat < lat))
	{
	  int rel_lat(((long long)(max_lat - min_lat))*(lon - min_lon)/(max_lon - min_lon) + min_lat);
	  if (rel_lat < lat)
	    area_cands[id] = !area_cands[id];
	  else if (rel_lat == lat)
	    //We are on a border segment.
	    area_definitives.insert(id);
	}
	else if ((min_lat == lat) && (max_lat == lat))
	  //We are on a horizontal border segment.
	  area_definitives.insert(id);
      }
      else if (min_lon == lon)
	//We are north of a node of the border.
	//We can safely count such a segment if and only if the node is
	//on its western end.
      {
	int min_lat(atoi(row[1]));
	if (min_lat < lat)
	  area_cands[id] = !area_cands[id];
	else if (min_lat == lat)
	  //We have hit a node of the border.
	  area_definitives.insert(id);
      }
    }
    else if (max_lon == lon)
    {
      int max_lat(atoi(row[3]));
      if (max_lat == lat)
	//We have hit a node of the border.
	area_definitives.insert(id);
      else if (min_lon == max_lon)
	//We are on a vertical border segment.
      {
	int min_lat(atoi(row[1]));
	if ((min_lat <= lat) && (lat <= max_lat))
	  area_definitives.insert(id);
      }
    }
    row = mysql_fetch_row(result);
  }
  
  while (mysql_fetch_row(result))
    ;
  mysql_free_result(result);
  for (set< int >::const_iterator it(area_definitives.begin());
       it != area_definitives.end(); ++it)
    result_set.insert(Area(*it));
  for (map< int, bool >::const_iterator it(area_cands.begin());
       it != area_cands.end(); ++it)
  {
    if (it->second)
      result_set.insert(Area(it->first));
  }
  return result_set;
}

set< int >& multiint_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< int >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

void multiint_to_null_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    mysql_query_null_wrapper(mysql, temp.str());
  }
  return;
}

set< Node >& multiint_to_multiNode_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< Node >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]) && (row[1]) && (row[2]))
    {
      result_set.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

set< Way >& multiint_to_multiWay_query
    (MYSQL* mysql, string prefix, string suffix, const set< int >& source, set< Way >& result_set)
{
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      Way way(atoi(row[0]));
      way.members.reserve(10);
      while ((row) && (row[0]) && (way.id == atoi(row[0])))
      {
	if ((row[1]) && (row[2]))
	{
	  unsigned int count((unsigned int)atol(row[1]));
	  if (way.members.capacity() < count)
	    way.members.reserve(count+10);
	  if (way.members.size() < count)
	    way.members.resize(count);
	  way.members[count-1] = atoi(row[2]);
	}
	row = mysql_fetch_row(result);
      }
      result_set.insert(way);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

set< Relation >& multiint_to_multiRelation_query
    (MYSQL* mysql, 
     string prefix1, string suffix1, string prefix2, string suffix2, string prefix3, string suffix3,
     const set< int >& source, set< Relation >& result_set)
{
  map< int, set< pair< int, int > > > node_members;
  map< int, set< pair< int, int > > > way_members;
  map< int, set< pair< int, int > > > relation_members;
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix1;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix1;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > nodes;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    nodes.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    nodes.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      node_members[id] = nodes;
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix2;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix2;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > ways;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    ways.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    ways.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      way_members[id] = ways;
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix3;
    temp<<" ("<<*it;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<") "<<suffix3;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      int id(atoi(row[0]));
      set< pair< int, int > > relations;
      while ((row) && (row[0]) && (id == atoi(row[0])))
      {
	if (row[1])
	{
	  if (row[2])
	    relations.insert
		(make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	  else
	    relations.insert
		(make_pair< int, int >(atoi(row[1]), 0));
	}
	row = mysql_fetch_row(result);
      }
      relation_members[id] = relations;
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  
  for (set< int >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    Relation relation(*it);
    relation.node_members = node_members[*it];
    relation.way_members = way_members[*it];
    relation.relation_members = relation_members[*it];
    result_set.insert(relation);
  }
  
  return result_set;
}

set< int >& multiNode_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Node >& source, set< int >& result_set)
{
  for (set< Node >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<it->id;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<it->id;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

set< int >& multiWay_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Way >& source, set< int >& result_set)
{
  for (set< Way >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<it->id;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<it->id;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

set< int >& multiRelation_to_multiint_query
    (MYSQL* mysql, string prefix, string suffix, const set< Relation >& source, set< int >& result_set)
{
  for (set< Relation >::const_iterator it(source.begin()); it != source.end(); )
  {
    ostringstream temp;
    temp<<prefix;
    temp<<" ("<<it->id;
    unsigned int i(0);
    while (((++it) != source.end()) && (i++ < 10000))
      temp<<", "<<it->id;
    temp<<") "<<suffix;
	
    MYSQL_RES* result(mysql_query_use_wrapper(mysql, temp.str()));
    if (!result)
      return result_set;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      result_set.insert(atoi(row[0]));
      row = mysql_fetch_row(result);
    }
    
    while (mysql_fetch_row(result))
      ;
    mysql_free_result(result);
  }
  return result_set;
}

//-----------------------------------------------------------------------------

typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

//-----------------------------------------------------------------------------

const unsigned int BLOCKSIZE = 512*1024;
const char* NODE_DATA = "/opt/osm_why_api/nodes.dat";
const char* NODE_IDX = "/opt/osm_why_api/nodes.b.idx";
const char* NODE_IDXA = "/opt/osm_why_api/nodes.1.idx";

//-----------------------------------------------------------------------------

set< Node >& multiint_to_multiNode_query(const set< int >& source, set< Node >& result_set)
{
  int nodes_dat_fd = open64(NODE_IDXA, O_RDONLY);
  if (nodes_dat_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno;
    runtime_error(temp.str(), cout);
    return result_set;
  }
  
  set< int > blocks;
  int16 idx_buf(0);
  for (set< int >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    lseek64(nodes_dat_fd, (*it)*sizeof(int16), SEEK_SET);
    read(nodes_dat_fd, &idx_buf, sizeof(int16));
    blocks.insert(idx_buf);
  }
  
  close(nodes_dat_fd);
  
  int32* buf_count = (int32*) malloc(sizeof(int) + BLOCKSIZE*sizeof(Node));
  Node* nd_buf = (Node*) &buf_count[1];
  if (!buf_count)
  {
    runtime_error("Bad alloc in node query", cout);
    return result_set;
  }
  
  nodes_dat_fd = open64(NODE_DATA, O_RDONLY);
  if (nodes_dat_fd < 0)
  {
    ostringstream temp;
    temp<<"open64: "<<errno;
    runtime_error(temp.str(), cout);
    return result_set;
  }
  
  for (set< int >::const_iterator it(blocks.begin()); it != blocks.end(); ++it)
  {
    lseek64(nodes_dat_fd, (int64)(*it)*(sizeof(int) + BLOCKSIZE*sizeof(Node)), SEEK_SET);
    read(nodes_dat_fd, buf_count, sizeof(int) + BLOCKSIZE*sizeof(Node));
    for (int32 i(0); i < buf_count[0]; ++i)
    {
      if (source.find(nd_buf[i].id) != source.end())
	result_set.insert(nd_buf[i]);
    }
  }
  
  close(nodes_dat_fd);
  
  free(buf_count);
  
  return result_set;
}