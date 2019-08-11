#ifndef SQL_TRACE_H
#define SQL_TRACE_H
#include <list>
#include <string>
#include <sqlite3.h>
#include "../../car-count/include/tracker.h"


class SqlTrace
{
public:
    SqlTrace(const std::string& dbDirectory, const std::string& dbFile, const std::string& tableName);
    ~SqlTrace();
    bool createTable();
    bool insertTrackState(long long frame, const std::list<Track>* trackList);

private:
    sqlite3*				m_dbHandle;
    std::string             m_tableName;
};

#endif // SQL_TRACE_H
