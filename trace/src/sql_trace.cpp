#include "../inc/sql_trace.h"

#include <iostream>
#include <boost/filesystem.hpp>

bool queryDbSingle(sqlite3* dbHandle, const std::string& sql, std::string& value);

SqlTrace::SqlTrace(const std::string& dbDirectory, const std::string& dbFile, const std::string& tableName) :
    m_dbHandle(nullptr),
    m_tableName(tableName) {

    // check db path existence
    namespace fs = boost::filesystem;
    fs::path dbPath(dbDirectory);
    if (!fs::exists(dbPath)) {
        std::cerr << "directory does not exist: " << dbDirectory << std::endl;
        throw "path does not exist";
    }

    // open or create db file
    std::string dbFilePath = dbDirectory + "/" + dbFile;
    int rc = sqlite3_open(dbFilePath.c_str(), &m_dbHandle);
    if (rc != SQLITE_OK) {
        std::cerr << "__FILE__, __LINE__: error opening db: " << dbFilePath << std::endl;
        std::cerr << "sqlite message: " << sqlite3_errmsg(m_dbHandle) << std::endl;
        sqlite3_close(m_dbHandle);
        m_dbHandle = nullptr;
        throw "cannot open db";
    }

    // TODO create table, if not exists
    // delete memberfcn createTable()
}


SqlTrace::~SqlTrace() {
    int rc = sqlite3_close(m_dbHandle);
    if (rc != SQLITE_OK)
        std::cerr << "__FILE__, __LINE__: error closing data base" << std::endl;
}


bool SqlTrace::createTable() {
    // table columns:
    // frame, trackId, x, y, w, h, confidence, length, velocity

    // create table, if not exist
    std::stringstream ss;
    ss << "create table if not exists " << m_tableName << " (frame int, id int, x int, y int, w int, h int, length int, velocity real);";
    std::string sqlStmt = ss.str();
    std::string answer;
    if (!queryDbSingle(m_dbHandle, sqlStmt, answer)) {
        std::cerr << "__FILE__, __LINE__: cannot create table: " << m_tableName << std::endl;
        return false;
    }

    return true;
}


bool SqlTrace::insertTrackState(long long frame, const std::list<Track>* trackList) {
    // check, if table exists

    if (trackList->size()) {

        // wrap in transaction
        std::string answer;
        std::string sqlStmt = "BEGIN TRANSACTION;";
        if (!queryDbSingle(m_dbHandle, sqlStmt, answer)) {
            std::cerr << "__FILE__, __LINE__: begin transaction failed" << std::endl;
            return false;
        }

        // for each track
        //   serialize table columns:
        //   frame, trackId, x, y, w, h, confidence, length, velocity

        for (auto track : *trackList) {
            std::stringstream ss;
            ss << "insert into " << m_tableName << " values ("
               << frame << ", "
               << track.getId() << ", "
               << track.getActualEntry().rect().x << ", "
               << track.getActualEntry().rect().y << ", "
               << track.getActualEntry().width() << ", "
               << track.getActualEntry().height() << ", "
               << static_cast<int>(track.getLength()) << ", "
               << track.getVelocity().x << ");";
            sqlStmt = ss.str();
            if (!queryDbSingle(m_dbHandle, sqlStmt, answer)) {
                std::cerr << "__FILE__, __LINE__: sql insertion failed" << std::endl;
                return false;
            }
        }

        // end transaction
        sqlStmt = "END TRANSACTION;";
        if (!queryDbSingle(m_dbHandle, sqlStmt, answer)) {
            std::cerr << "__FILE__, __LINE__: end transaction failed" << std::endl;
            return false;
        }
    } // end if trackList->size

    return true;
}


bool queryDbSingle(sqlite3* dbHandle, const std::string& sql, std::string& value) {
    bool success = false;
    sqlite3_stmt *stmt;
    // empty value indicate error
    value.clear();

    int rc = sqlite3_prepare_v2(dbHandle, sql.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        int step = SQLITE_ERROR;
        int nRow = 0;
        do
        {
            step = sqlite3_step(stmt);

            switch (step) {
            case SQLITE_ROW:
                {
                    // one result expected: take first row only and discard others
                    if (nRow == 0)
                    {
                        int nCol = sqlite3_column_count(stmt);
                        nCol = 0; // one result expected: take first column only
                        if (sqlite3_column_type(stmt, nCol) == SQLITE_NULL)
                            std::cerr << __LINE__ << " NULL value in table" << std::endl;
                        else
                            value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, nCol));
                    }
                }
                break;
            case SQLITE_DONE: break;
            default:
                std::cerr << __LINE__ << "Error executing step statement" << std::endl;
                break;
            }
            ++nRow;
        } while (step != SQLITE_DONE);

        rc = sqlite3_finalize(stmt);
        if (rc == SQLITE_OK)
            success = true;
        else
            success = false;
    }

    else // sqlite3_prepare != OK
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(dbHandle) << std::endl;
        rc = sqlite3_finalize(stmt);
        success = false;
    }
    return success;
}
