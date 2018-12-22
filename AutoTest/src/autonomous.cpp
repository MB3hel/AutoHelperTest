/**
 * autonomous.cpp
 * See autonomous.hpp for details.
 *
 * Copyright (c) 2018 FRC Team 2655 - The Flying Platypi
 * See LICENSE file for details
 */

#include "autonomous.hpp"

#include <chrono>
#include <sstream>
#include <regex>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace team2655;

////////////////////////////////////////////////////////////////////////
/// AutoCommand
////////////////////////////////////////////////////////////////////////

int64_t AutoCommand::currentTimeMillis(){
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

bool AutoCommand::hasTimedOut(){
	return timeout > 0 && (currentTimeMillis()  - startTime >= timeout);
}

bool AutoCommand::hasStarted(){
	return _hasStarted;
}

bool AutoCommand::isComplete(){
	return _isComplete;
}

void AutoCommand::setTimeout(int timeoutMs){
	this->timeout = timeoutMs;
}

int AutoCommand::getTimeout(){
	return this->timeout;
}

void AutoCommand::doStart(std::string commandName, std::vector<std::string> args){
	this->commandName = commandName;
	this->arguments = args;
	this->startTime = currentTimeMillis();
	this->_hasStarted = true;
	// Call the start function to be used by custom commands
	start(commandName, args);
}

void AutoCommand::doProcess(){
	// If the command has timed out complete the command
	if(hasTimedOut()){
		complete();
	}
	// If the command is completed or not started do not do anything
	if(_isComplete || !_hasStarted)
		return;
	// Call the process function to be used by custom commands
	process();
}

void AutoCommand::complete(){
	this->_isComplete = true;
	// Call the complete function to be used by custom commands
	handleComplete();
}

////////////////////////////////////////////////////////////////////////
/// BackgroundAutoCommand
////////////////////////////////////////////////////////////////////////

void BackgroundAutoCommand::doUpdateArgs(std::string commandName, std::vector<std::string> args){
	updateArgs(commandName, args);
}

////////////////////////////////////////////////////////////////////////
/// AutoManager
////////////////////////////////////////////////////////////////////////

std::vector<std::string> AutoManager::split(const std::string& s, char delimiter){
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)){
		tokens.push_back(token);
	}
	return tokens;
}

// Registration methods

void AutoManager::registerCommand(CmdCreator creator, std::string name){
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	// Only one command *or* background command can have a key.
	if(backgroundCommands.find(name) == backgroundCommands.end() && registeredCommands.find(name) == registeredCommands.end()){
		registeredCommands[name] = creator;
	}else{
		std::cerr << "Cannot register command with name \"" << name << "\". A command is already registered with that name." << std::endl;
	}
}

void AutoManager::registerCommand(CmdCreator creator, std::vector<std::string> names){
	for(size_t i = 0; i < names.size(); ++i){
		registerCommand(creator, names[i]);
	}
}

void AutoManager::unregisterAll(){
	registeredCommands.clear();
	backgroundCommands.clear();
	bgCommandTypes.clear();
	uniqueBgCommands.clear();
}

// Script management

bool AutoManager::loadScript(std::string fileName){

	clearCommands();

	std::ifstream scriptFile;
	scriptFile.open(fileName);

	if(!scriptFile.good()){
		std::cerr << "Script file: \"" << fileName << "\" not found." << std::endl;
		scriptFile.close();
		return false; // Some error accessing the file
	}

	// Get the file contents
	std::stringstream fileContents;
	fileContents << scriptFile.rdbuf();
	scriptFile.close();

	// Remove spaces before and after commas
	// Commands and args cannot start or end with spaces
	//std::string csvData = std::regex_replace(fileContents.str(), std::regex(",\\ +"), ",");

	// Standardize line endings convert any line ending int '\n'
	std::string csvData = std::regex_replace(fileContents.str(), std::regex("(\r\n|\r|\n)"), "\n");

	std::vector<std::string> lines = split(csvData, '\n'); // Separate each line

	// Separate each column of the CSV
	for(size_t i = 0; i < lines.size(); i++){
		std::vector<std::string> columns = split(lines[i], ','); // All the columns in the CSV
		loadedCommands.push_back(columns[0]); // This is the command
		columns.erase(columns.begin()); // Remove the command from the list of columns. This will leave only arguments
		loadedArguments.push_back(columns);
	}

	// Reset
	currentCommandIndex = -1;
	currentCommand.release();

	return true;
}

void AutoManager::addCommand(std::string command, std::vector<std::string> arguments, int pos){

	// Any position beyond the end of the vector is converted to -1 (aka the end)
	if((pos > ((int)loadedCommands.size())) || pos < -1)
		pos = -1;

	// Add to the end otherwise insert at a position
	if(pos == -1){
		loadedCommands.push_back(command);
		loadedArguments.push_back(arguments);
	}else{
		loadedCommands.insert(loadedCommands.begin() + pos, command);
		loadedArguments.insert(loadedArguments.begin() + pos, arguments);
	}

}

void AutoManager::addCommands(std::vector<std::string> commands, std::vector<std::vector<std::string>> arguments, int pos){

	if(commands.size() != arguments.size()){
		std::cerr << "AutoManagerError: addCommands: must have same number of commands and arguments" << std::endl;
		return;
	}

	// Any position beyond the end of the vector is converted to -1 (aka the end)
	if(pos > ((int)loadedCommands.size()) || pos < -1)
		pos = -1;

	loadedCommands.insert((pos == -1) ? loadedCommands.end() : loadedCommands.begin() + pos,
			              commands.begin(),
						  commands.end());
	loadedArguments.insert((pos == -1) ? loadedArguments.end() : loadedArguments.begin() + pos,
						   arguments.begin(),
						   arguments.end());
}

size_t AutoManager::loadedCommandCount(){
	return loadedCommands.size();
}

void AutoManager::clearCommands(){
	killAuto();
	loadedCommands.clear();
	loadedArguments.clear();
	currentCommandIndex = -1;
}

// Perform actions

void AutoManager::handleNextBgCommands(){
	// If the next command is a background command process background commands until
	//    there are no more commands or until the next is not a background command
	std::string key = loadedCommands[currentCommandIndex];
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
	while(backgroundCommands.find(key) != backgroundCommands.end()){
		uniqueBgCommands[backgroundCommands[key]]->doUpdateArgs(key, loadedArguments[currentCommandIndex]);
		currentCommandIndex++;

		// Get next key. This will repeat if the next key is for a background command
		key = loadedCommands[currentCommandIndex];
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
	}
}

bool AutoManager::process(){
	if(loadedCommandCount() < 1)
		return false; // At the end of the non-existent script. Consider this the same as finished with a script

	bool result = true;
	// If the current command is done of there is no current command
	if(currentCommand.get() == nullptr || currentCommand.get()->isComplete()){
		// Move on to the next command
		currentCommandIndex++;
		currentCommand.release();

		// If this is the end of the loadedCommands will return false, but still needs to reach processing of bg commands
		if(currentCommandIndex >= ((int)loadedCommands.size())){
			result =  false;
		}else{
			// Handle the next several (if any) background commands
			handleNextBgCommands();

			// If this is the end of the loadedCommands will return false, but still needs to reach processing of bg commands
			if(currentCommandIndex >= ((int)loadedCommands.size())){
				result =  false;
			}else{
				// Get next command
				std::string key = loadedCommands[currentCommandIndex];
				std::transform(key.begin(), key.end(), key.begin(), ::tolower);
				if(registeredCommands.find(key) != registeredCommands.end()){
					currentCommand = registeredCommands[key](); // Run the creator for this command
				}else{
					std::cerr << "WARNING: No command registered for key \"" << key << "\". Command will be skipped." << std::endl;
				}
			}
		}

		
	}

	// start or process the current command (if it were completed it will have been handled above)
	if(currentCommand.get() != nullptr){
		if(!currentCommand.get()->hasStarted()){
			currentCommand.get()->doStart(loadedCommands[currentCommandIndex], loadedArguments[currentCommandIndex]);
			currentCommand.get()->process();
		}else{
			currentCommand.get()->doProcess();
		}
	}

	// Process background commands
	for (auto const &element : uniqueBgCommands){
		if(element.get()->shouldProcess())
			element.get()->process();
	}

	return result; // True if there are more commands to handle in the script (this could be false but bg commands still need to run)
}

void AutoManager::killAuto(){
	if(currentCommand.get() != nullptr)
		currentCommand.get()->complete();
	currentCommandIndex = loadedCommands.size();
	currentCommand.release();

	// Kill all background commands
	for (auto const &element : uniqueBgCommands){
		element.get()->kill();
	}
}
