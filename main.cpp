#include "general.h"
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "deleteoldreports.h"
#include "yddconf.h"
#include "dbconn.h"

#include "ydrgetversion.h"
#include "ydtask.h"

#include <mysql++/mysql++.h>

void timerHandler(const boost::system::error_code& /*e*/)
{
    std::cout << "Timer" << std::endl;
}

int main()
{
    using namespace ydd;
    using namespace std;
    string token = "c9f13bf86c694e629440c6d56dd29b1e";

    // what is 'reactor pattern?
    try
    {
	DbConn dbc;
	YdTask ydt(dbc, 173025, 1);
	ydt.log(YdTask::info, "Test");
	boost::asio::io_service io_service;
	//DeleteOldReports dro(token, io_service);
	//io_service.post(boost::bind(&DeleteOldReports::run, &dro));
	YdrGetVersion r(token, io_service, true, NULL);
	r.run();
	//io_service.run();
	std::cout << r.getJsonResponse() << std::endl;
    }
    catch (std::exception& e)
    {
	std::cout << "Exception: " << e.what() << endl;
    }

    return 0;
}
