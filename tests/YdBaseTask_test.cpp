#include "../ydbasetask.h"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace ydd;

class TestYdBaseTask : public YdBaseTask
{
    public:
	TestYdBaseTask(DbConn& dbc, DbConn::UserIdType userId, DbConn::TaskIdType taskId) : 
	    YdBaseTask(dbc, userId, taskId)
	{
	    using namespace mysqlpp;
	    dbc_.switchUserDb(userId_);
	    Query query = dbc_.get().query();
	    query << "DROP PROCEDURE IF EXISTS `sp_fill_test_tasks_phrases_set`";
	    query.execute();
	    query << 
		"CREATE PROCEDURE `sp_fill_test_tasks_phrases_set`"
		    "(IN `p_first` INT UNSIGNED, IN `p_last` INT UNSIGNED, "
		    " IN `p_finished` BOOLEAN, IN `p_taskid` INT UNSIGNED) "
		"BEGIN\r\n"
		"\tDECLARE i INT;\r\n"
		"\tDECLARE phr VARCHAR(32);\r\n"
		"\tSET i = p_first;\r\n"
		"\tWHILE i <= p_last DO\r\n"
		"\t\tSET phr = CONCAT('phrase', i);\r\n"
		"\t\tINSERT INTO tasks_phrases(`taskid`, `phrase`, `finished`) \r\n"
		"\t\t\tVALUES (p_taskid, phr, p_finished);\r\n"
		"\t\tSET i = i + 1;\r\n"
		"\tEND WHILE;\r\n"
		"END";
	    query.execute();
	}

	virtual ~TestYdBaseTask()
	{
	}

	void test_logQuery_simple(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "Simple message", NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, 'Simple message', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_logQuery_oneDoubleQuote(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "One double \" quote", NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, 'One double \\\" quote', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_logQuery_oneSingleQuote(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "One ' single quote", NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, 'One \\\' single quote', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_logQuery_combinedQuotes1(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "'Comb'ined \"quotes\" 1", NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, '\\\'Comb\\\'ined \\\"quotes\\\" 1', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_logQuery_combinedQuotes2(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "\"Combined\" 'quotes 2\"", NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, '\\\"Combined\\\" \\\'quotes 2\\\"', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_logQuery_sqlInjection1(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "select * from tasks_phrases", NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, 'select * from tasks_phrases', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_logQuery_sqlInjection2(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "select * from `tasks_phrases where `id` = 0", NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, 'select * from `tasks_phrases where `id` = 0', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_logQuery_specialchars(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "±§!@#$%^&*()_+№;:`{?-+=/][}{<>~`", NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, '±§!@#$%^&*()_+№;:`{?-+=/][}{<>~`', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_log_sqlInjection1(mysqlpp::Query query)
	{
	    BOOST_REQUIRE_NO_THROW(log(info, "select * from tasks_phrases"));
	}

	void test_log_sqlInjection2(mysqlpp::Query query)
	{
	    BOOST_REQUIRE_NO_THROW(log(info, "select * from `tasks_phrases where `id` = 0"));
	}

	void test_log_specialchars(mysqlpp::Query query)
	{
	    BOOST_REQUIRE_NO_THROW(log(info, "±§!@#$%^&*()_+№;:`{?-+=/][}{<>~`"));
	}

	void test_logQuery_XSS1(mysqlpp::Query query)
	{
	    std::string q;
	    logQuery(query, info, "<script type=\"text/javascript\">alert('XSS');</script>", 
		    NULL, &q);
	    BOOST_REQUIRE_EQUAL(query.str(), 
		    "CALL sp_add_log_record(1, 0, "
		    "'<script type=\\\"text/javascript\\\">alert(\\\'XSS\\\');</script>', NULL, @ret);"
		    "SELECT @ret;");
	}

	void test_log_XSS1(mysqlpp::Query query)
	{
	    BOOST_REQUIRE_NO_THROW(log(info, "<script type=\"text/javascript\">alert('XSS');</script>"));
	}

	/*
	DELETE FROM `phrases_keywords`;
	ALTER TABLE `phrases_keywords` AUTO_INCREMENT = 1;
	DELETE FROM `tasks_phrases`;
	ALTER TABLE `tasks_phrases` AUTO_INCREMENT = 1
	*/
};

struct FxYdBaseTask
{
    FxYdBaseTask() :
	dbc(),
	userId(50),
	taskId(1),
	tydt(dbc, userId, taskId),
	conn(dbc.get()),
	query(conn.query())
    {
    }

    DbConn dbc;
    DbConn::UserIdType userId;
    DbConn::TaskIdType taskId;
    TestYdBaseTask tydt;
    mysqlpp::Connection& conn;
    mysqlpp::Query query;
};

BOOST_FIXTURE_TEST_CASE(logQuery_simple, FxYdBaseTask)
{
    tydt.test_logQuery_simple(query);
}

BOOST_FIXTURE_TEST_CASE(logQuery_oneDoubleQuote, FxYdBaseTask)
{
    tydt.test_logQuery_oneDoubleQuote(query);
}

BOOST_FIXTURE_TEST_CASE(logQuery_oneSingleQuote, FxYdBaseTask)
{
    tydt.test_logQuery_oneSingleQuote(query);
}

BOOST_FIXTURE_TEST_CASE(logQuery_combinedQuotes1, FxYdBaseTask)
{
    tydt.test_logQuery_combinedQuotes1(query);
}

BOOST_FIXTURE_TEST_CASE(logQuery_combinedQuotes2, FxYdBaseTask)
{
    tydt.test_logQuery_combinedQuotes2(query);
}

BOOST_FIXTURE_TEST_CASE(logQuery_sqlInjection1, FxYdBaseTask)
{
    tydt.test_logQuery_sqlInjection1(query);
}

BOOST_FIXTURE_TEST_CASE(logQuery_sqlInjection2, FxYdBaseTask)
{
    tydt.test_logQuery_sqlInjection2(query);
}

BOOST_FIXTURE_TEST_CASE(logQuery_specialchars, FxYdBaseTask)
{
    tydt.test_logQuery_specialchars(query);
}

BOOST_FIXTURE_TEST_CASE(log_sqlInjection1, FxYdBaseTask)
{
    tydt.test_log_sqlInjection1(query);
}

BOOST_FIXTURE_TEST_CASE(log_sqlInjection2, FxYdBaseTask)
{
    tydt.test_log_sqlInjection2(query);
}

BOOST_FIXTURE_TEST_CASE(log_specialchars, FxYdBaseTask)
{
    tydt.test_log_specialchars(query);
}

BOOST_FIXTURE_TEST_CASE(logQuery_XSS1, FxYdBaseTask)
{
    tydt.test_logQuery_XSS1(query);
}

BOOST_FIXTURE_TEST_CASE(log_XSS1, FxYdBaseTask)
{
    tydt.test_log_XSS1(query);
}

