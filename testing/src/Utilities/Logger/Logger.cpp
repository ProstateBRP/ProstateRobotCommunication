//============================================================================
// Name        : Logger.cpp
// Author      : Produced in the WPI AIM Lab
// Description : This file is used for logging information from the system to a text file
//============================================================================

#include "Logger.hpp"

Logger::Logger()
{
	_timer = Timer();
	_filename = GetFileName();
	_file.open(_filename, fstream::in | fstream::out | fstream::trunc);
	_logLevelSeverity = logger::INFO;
	_timer.tic();
}

// Determine a new filename to use
// Do not use a file name already on the system
string Logger::GetFileName()
{
	bool validFileNameFound = false;

	string filename = "";
	string fileheader = "logs/Log_";
	int counter = 1;
	while (!validFileNameFound)
	{
		filename = fileheader + to_string(counter) + ".txt";
		ifstream f(filename);
		if (!f.good())
		{
			validFileNameFound = true;
		}

		counter++;
	}
	cout << "Writing to file: " << filename << endl;
	return filename;
}

// Main Logger Function
// Writes Data to the output file given the log severity
void Logger::Log(const string &logData, int severity, bool printToConsole)
{
	// Get current processor time
	_timer.toc();
	unsigned long long time_micro_seconds = _timer.time();

	// MUST BE IN SAME ORDER AS LOG LEVEL DEFINES
	vector<string> severities = {"DEBUG", "INFO", "WARNING", "ERROR",
								 "CRITICAL", "DATA"};

	// If within current log levels write data to log file
	pthread_mutex_lock(&logMutex);
	if (severity >= _logLevelSeverity)
	{
		_file << "[" << time_micro_seconds << "] " << severities[severity]
			  << " --: " << logData << endl;
	}

	if (printToConsole)
	{
		cout << "[" << time_micro_seconds << "] " << severities[severity]
			 << " --: " << logData << endl;
	}
	pthread_mutex_unlock(&logMutex);
}
// Overloaded log method to log data using the actual time received from the navigation software
void Logger::Log(const std::string &logData, std::string actual_time, int severity, bool printToConsole)
{
	// Updating the actual time based on what the navigation software has sent
	SetActualTime(actual_time);

	// MUST BE IN SAME ORDER AS LOG LEVEL DEFINES
	vector<std::string> severities = {"DEBUG", "INFO", "WARNING", "ERROR",
									  "CRITICAL", "DATA"};

	// If within current log levels write data to log file
	pthread_mutex_lock(&logMutex);
	if (severity >= _logLevelSeverity)
	{
		_file << "[" << GetActualTime() << "] " << severities[severity]
			  << " --: " << logData << endl;
	}

	if (printToConsole)
	{
		cout << "[" << GetActualTime() << "] " << severities[severity]
			 << " --: " << logData << endl;
	}
	pthread_mutex_unlock(&logMutex);
}
// Overloaded log method to log TRANSFORM data
void Logger::Log(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &logData, string name, string actual_time, int severity, bool printToConsole)
{
	// Updating the actual time based on what the navigation software has sent
	SetActualTime(actual_time);
	// MUST BE IN SAME ORDER AS LOG LEVEL DEFINES
	vector<std::string> severities = {"DEBUG", "INFO", "WARNING", "ERROR",
									  "CRITICAL", "DATA"};

	// If within current log levels write data to log file
	pthread_mutex_lock(&logMutex);

	// Rounding numbers to two decimals
	Eigen::Matrix<double, 4, 4, Eigen::DontAlign> log_data_rounded = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Zero();
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			log_data_rounded(i, j) = std::round(logData(i, j) * 100) / 100;
		}
	}
	if (severity >= _logLevelSeverity)
	{
		_file << "[" << GetActualTime() << "] " << severities[severity]
			  << " --:" << name << ":\n"
			  << "[" << log_data_rounded(0, 0) << "," << log_data_rounded(0, 1) << "," << log_data_rounded(0, 2) << "," << log_data_rounded(0, 3) << "\n"
			  << log_data_rounded(1, 0) << "," << log_data_rounded(1, 1) << "," << log_data_rounded(1, 2) << "," << log_data_rounded(1, 3) << "\n"
			  << log_data_rounded(2, 0) << "," << log_data_rounded(2, 1) << "," << log_data_rounded(2, 2) << "," << log_data_rounded(2, 3) << "\n"
			  << log_data_rounded(3, 0) << "," << log_data_rounded(3, 1) << "," << log_data_rounded(3, 2) << "," << log_data_rounded(3, 3) << "]\n";
	}

	if (printToConsole)
	{
		cout << "[" << GetActualTime() << "] " << severities[severity]
			 << " --:" << name << ":\n"
			 << "[" << log_data_rounded(0, 0) << "," << log_data_rounded(0, 1) << "," << log_data_rounded(0, 2) << "," << log_data_rounded(0, 3) << "\n"
			 << log_data_rounded(1, 0) << "," << log_data_rounded(1, 1) << "," << log_data_rounded(1, 2) << "," << log_data_rounded(1, 3) << "\n"
			 << log_data_rounded(2, 0) << "," << log_data_rounded(2, 1) << "," << log_data_rounded(2, 2) << "," << log_data_rounded(2, 3) << "\n"
			 << log_data_rounded(3, 0) << "," << log_data_rounded(3, 1) << "," << log_data_rounded(3, 2) << "," << log_data_rounded(3, 3) << "]\n";
	}
	pthread_mutex_unlock(&logMutex);
}
// Overloaded log method to log TRANSFORM data
void Logger::Log(const igtl::Matrix4x4 &logData, string name, string actual_time, int severity, bool printToConsole)
{
	// Updating the actual time based on what the navigation software has sent
	SetActualTime(actual_time);
	// MUST BE IN SAME ORDER AS LOG LEVEL DEFINES
	vector<std::string> severities = {"DEBUG", "INFO", "WARNING", "ERROR",
									  "CRITICAL", "DATA"};

	// If within current log levels write data to log file
	pthread_mutex_lock(&logMutex);

	igtl::Matrix4x4 log_data_rounded;
	// Rounding numbers to two decimals
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			log_data_rounded[i][j] = std::round(logData[i][j] * 100) / 100;
		}
	}

	if (severity >= _logLevelSeverity)
	{
		_file << "[" << GetActualTime() << "] " << severities[severity]
			  << " --:" << name << ":\n"
			  << "[" << log_data_rounded[0][0] << "," << log_data_rounded[0][1] << "," << log_data_rounded[0][2] << "," << log_data_rounded[0][3] << "\n"
			  << log_data_rounded[1][0] << "," << log_data_rounded[1][1] << "," << log_data_rounded[1][2] << "," << log_data_rounded[1][3] << "\n"
			  << log_data_rounded[2][0] << "," << log_data_rounded[2][1] << "," << log_data_rounded[2][2] << "," << log_data_rounded[2][3] << "\n"
			  << log_data_rounded[3][0] << "," << log_data_rounded[3][1] << "," << log_data_rounded[3][2] << "," << log_data_rounded[3][3] << "]\n";
	}

	if (printToConsole)
	{
		cout << "[" << GetActualTime() << "] " << severities[severity]
			 << " --:" << name << ":\n"
			 << "[" << log_data_rounded[0][0] << "," << log_data_rounded[0][1] << "," << log_data_rounded[0][2] << "," << log_data_rounded[0][3] << "\n"
			 << log_data_rounded[1][0] << "," << log_data_rounded[1][1] << "," << log_data_rounded[1][2] << "," << log_data_rounded[1][3] << "\n"
			 << log_data_rounded[2][0] << "," << log_data_rounded[2][1] << "," << log_data_rounded[2][2] << "," << log_data_rounded[2][3] << "\n"
			 << log_data_rounded[3][0] << "," << log_data_rounded[3][1] << "," << log_data_rounded[3][2] << "," << log_data_rounded[3][3] << "]\n";
	}
	pthread_mutex_unlock(&logMutex);
}

// Overloaded log method to log VECTOR data
void Logger::Log(const Eigen::Matrix<double, 4, 1, Eigen::DontAlign> &logData, string name, string actual_time, int severity, bool printToConsole)
{
	// Updating the actual time based on what the navigation software has sent
	SetActualTime(actual_time);
	// MUST BE IN SAME ORDER AS LOG LEVEL DEFINES
	vector<std::string> severities = {"DEBUG", "INFO", "WARNING", "ERROR",
									  "CRITICAL", "DATA"};

	// If within current log levels write data to log file
	pthread_mutex_lock(&logMutex);
	Eigen::Matrix<double, 4, 1, Eigen::DontAlign> log_data_rounded = Eigen::Matrix<double, 4, 1, Eigen::DontAlign>::Zero();
	// Rounding numbers to two decimals
	for (int i = 0; i < 4; i++)
	{

		log_data_rounded(i) = std::round(logData(i) * 100) / 100;
	}

	if (severity >= _logLevelSeverity)
	{
		_file << "[" << GetActualTime() << "] " << severities[severity]
			  << " --:" << name << ":\n"
			  << "[" << log_data_rounded(0) << "," << log_data_rounded(1) << "," << log_data_rounded(2) << "," << log_data_rounded(3) << "]" << endl;
	}

	if (printToConsole)
	{
		cout << "[" << GetActualTime() << "] " << severities[severity]
			 << " --:" << name << ":\n"
			 << "[" << log_data_rounded(0) << "," << log_data_rounded(1) << "," << log_data_rounded(2) << "," << log_data_rounded(3) << "]" << endl;
	}
	pthread_mutex_unlock(&logMutex);
}
// Read in the last n number of lines to read
vector<vector<string>> Logger::ReadLogData(int numberOfLinesToRead)
{
	vector<vector<string>> result;

	// Given that the files is open
	if (_file.is_open())
	{

		// Go to one spot before the end of the file
		_file.seekg(-1, ios_base::end);

		// Loop specific parameters
		int firstCharacterRead = true;
		bool keepLooping = true;
		int line_counter = 1;

		// While we're still looping
		pthread_mutex_lock(&logMutex);
		while (keepLooping)
		{
			// Get current byte's data
			char ch;
			_file.get(ch);

			// If the data was at or before the 0th byte OR the line count is greater than the numberOfLinesToRead
			if ((int)_file.tellg() <= 1 || line_counter > numberOfLinesToRead)
			{
				// Stop Looping
				keepLooping = false;
			}

			// If a new line was found0, then save the current line
			else if (ch == '\n' && !firstCharacterRead)
			{
				// Save our position before reading the line in the file
				int before = (int)_file.tellg();

				// Read the current line
				string lastLine;
				getline(_file, lastLine);

				// Get the file position after reading the line in the file
				int after = (int)_file.tellg();

				// Seek back to the start of the line
				_file.seekg(-1 * (after - before) - 2, ios_base::cur);

				// Add data to results array
				// Position of first character in the last line is recorded as a unique identifier for the Web GUI
				vector<string> temp = {to_string(before), lastLine};
				result.push_back(temp);

				// Increment the line counter by one
				line_counter += 1;
			}

			// If the data was neither a newline nor before the start of the file
			else
			{
				// Then seek forward one byte
				_file.seekg(-2, ios_base::cur);
			}

			firstCharacterRead = false;
		}

		// Go back to the end of the file
		_file.seekg(0, ios_base::end);
	}
	pthread_mutex_unlock(&logMutex);

	return result;
}

// Set the Logger level from the HTTP Server side information
void Logger::SetLogLevel(int severity)
{
	_logLevelSeverity = severity;
}
// Set actual time based on the time sent from the navigation software
void Logger::SetActualTime(string actualTime)
{
	try
	{
		this->_actualTime = actualTime.substr(0, 2) + ":" + actualTime.substr(2, 2) + ":" + actualTime.substr(4, 2) + +":" +
							actualTime.substr(6, std::string::npos);
	}
	catch (...)
	{
		Logger &log = Logger::GetInstance();
		log.Log("Query ID is not consistent with the desired definition.\n",1,true);
		this->_actualTime = "00:00:00:000000";
	}
}
