/**
 * autonomous.hpp
 * Contains FRC Team 2655's autonomous helper code
 * Team 2655's CSV script based autonomous
 *
 * @author Marcus Behel
 * @version 1.0.0 9-20-2018 Initial Version
 *
 * Copyright (c) 2018 FRC Team 2655 - The Flying Platypi
 * See LICENSE file for details
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <algorithm>

namespace team2655{


class AutoCommand{
protected:
	bool _hasStarted = false;
	bool _isComplete = false;
	int timeout = 0;
	int64_t startTime = 0;
	std::string commandName;
	std::vector<std::string> arguments;

	/**
	 * Get the current time as milliseconds from the epoch
	 * @return Number of milliseconds since the epoch
	 */
	static int64_t currentTimeMillis();

	/**
	 * Check if the command has timed out
	 * @return
	 */
	bool hasTimedOut();

public:

	/**
	 * Start the command
	 * @param args THe arguments provided for the command
	 */
	void doStart(std::string commandName, std::vector<std::string> args);

	/**
	 * Periodic actions for the command
	 */
	void doProcess();

	/**
	 * Complete / finish the command
	 */
	void complete();

	/**
	 * Has the command been started (init called)
	 * @return true if started, false if not
	 */
	bool hasStarted();

	/**
	 * Has the command completed (complete called)
	 * @return true if completed, false if not
	 */
	bool isComplete();

	/**
	 * Set the timeout for this command
	 * @param timeoutMs The timeout in milliseconds
	 */
	void setTimeout(int timeoutMs);

	/**
	 * Get the timeout for this command
	 * @return The timeout for this command in milliseconds
	 */
	int getTimeout();

	/**
	 * Handle when the command starts
	 * @param args The arguments provided for the command
	 */
	virtual void start(std::string commandName, std::vector<std::string> args) = 0;

	/**
	 * Handle periodic functions for the command
	 */
	virtual void process() = 0;

	/**
	 * Handle command complete
	 */
	virtual void handleComplete() = 0;

	virtual ~AutoCommand() {  }
};

class BackgroundAutoCommand{
public:
	void doUpdateArgs(std::string commandName, std::vector<std::string> args);
	virtual void updateArgs(std::string commandName, std::vector<std::string> args) = 0;
	virtual void process() = 0;
	virtual void kill() = 0;
	virtual bool shouldProcess() = 0;

	virtual ~BackgroundAutoCommand(){}
};

typedef std::unique_ptr<AutoCommand> CmdPointer;
typedef std::function<CmdPointer()> CmdCreator;
typedef std::unique_ptr<BackgroundAutoCommand> BgCmdPointer;

template<class T>
CmdPointer CommandCreator(){
	if(!std::is_base_of<AutoCommand, T>::value){
		std::cerr << "WARNING: Creator cannot create command. Given type is not a valid AutoCommand." << std::endl;
		return CmdPointer(nullptr);
	}
	return CmdPointer(new T());
}

/**
 * A class to handle loading of autonomous command scripts and running AutoCommand objects
 */
class AutoManager{
protected:
	// Data for the current script
	std::vector<std::string> loadedCommands;
	std::vector<std::vector<std::string>> loadedArguments;
	size_t currentCommandIndex = -1;
	CmdPointer currentCommand{nullptr};

	// Registered commands. Used to get a command from a command name (string)
	std::unordered_map<std::string, CmdCreator> registeredCommands;
	std::unordered_map<std::string, size_t> backgroundCommands; // This handles mapping from string to index in uniqueBgCommands
	std::vector<BgCmdPointer> uniqueBgCommands; // Only one per registered type so if registered with 5 names will not be run 5 times
	std::vector<std::string> bgCommandTypes;

	/**
	 * Split a string by a character delimiter
	 * @param s The string to split
	 * @param delimiter The delimiter to split by
	 * @return A vector of strings (the specific segments)
	 */
	std::vector<std::string> split(const std::string& s, char delimiter);

	/**
	 * If the next command is a background command update its values.
	 * This will happen for *all* consectutive background commands.
	 */
	void handleNextBgCommands();

public:

	/**
	 * Register a command with the AutoManager
	 * @param creator The CommandCreator for the command (use CommandCreator<T>)
	 * @param name The name to register the command with
	 */
	void registerCommand(CmdCreator creator, std::string name);

	/**
	 * Register a command with the AutoManager
	 * @param creator The CommandCreator for the command (use CommandCreator<T>)
	 * @param names A set of names to register the command with
	 */
	void registerCommand(CmdCreator creator, std::vector<std::string> names);

	/**
	 * Register a background command with the auto manager
	 * @param name The name to register the command with
	 */
	template<class T>
	void registerBackgroundCommand(std::string name){
		if(!std::is_base_of<BackgroundAutoCommand, T>::value){
			std::cerr << "WARNING: Cannot register background command. Given type is not a valid BackgroundAutoCommand." << std::endl;
			return;
		}

		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		// Only one command *or* background command can have a key.
		if(backgroundCommands.find(name) == backgroundCommands.end() && registeredCommands.find(name) == registeredCommands.end()){
			const std::type_info &t = typeid(T); // Type of command
			auto it = std::find(bgCommandTypes.begin(), bgCommandTypes.end(), t.name()); // Is this type already added
			if(it == bgCommandTypes.end()){
				// First one of this type registered. Add to uniqueCmds vector and register mapping to index
				bgCommandTypes.push_back(t.name());
				uniqueBgCommands.push_back(BgCmdPointer(new T()));
				backgroundCommands[name] = uniqueBgCommands.size() - 1;
			}else{
				// This type already registered
				backgroundCommands[name] = it - bgCommandTypes.begin(); // Map new name key to existing command in uniqueCmds vector
			}
		}else{
			std::cerr << "Cannot register command with name \"" << name << "\". A command is already registered with that name." << std::endl;
		}
	}

	/**
	 * Register a background command with the auto manager
	 * @param names A set of names to register the command with
	 */
	template<class T>
	void registerBackgroundCommand(std::vector<std::string> names){
		if(!std::is_base_of<BackgroundAutoCommand, T>::value){
			std::cerr << "WARNING: Cannot register background command. Given type is not a valid BackgroundAutoCommand." << std::endl;
			return;
		}

		for(size_t i = 0; i < names.size(); ++i){
			registerBackgroundCommand<T>(names[i]);
		}
	}

	/**
	 * Unregister all commands and background commands
	 */
	void unregisterAll();

	/**
	 * Load an autonomous CSV script from the script path
	 * @param fileName The full path to the script to load
	 * @return Was the script successfully loaded
	 */
	bool loadScript(std::string fileName);

	/**
	 * Add a command to autonomous
	 * @param command The command
	 * @param arguments The arguments for the command
	 * @param pos The position to insert the command at (-1 for the end of the loaded script).
	 */
	void addCommand(std::string command, std::vector<std::string> arguments, int pos = -1);

	/**
	 * Add a set of commands to the end of autonomous
	 * @param commands The commands (command names) of the commands to execute
	 * @param arguments The arguments for each command
	 * @param pos THe position to insert the command at (-1 for the end of the loaded script).
	 */
	void addCommands(std::vector<std::string> commands, std::vector<std::vector<std::string>> arguments, int pos = -1);

	/**
	 * Remove all loaded commands
	 */
	void clearCommands();

	/**
	 * Get the number of loaded (and added) commands
	 * @return The number of commands loaded from a script (and added manually)
	 */
	size_t loadedCommandCount();

	/**
	 * Process the autonomous commands.
	 * Handles the current command, any background commands, and moving between commands.
	 * @return True if there are any commands (excluding background commands) that have not finished
	 */
	bool process();

	/**
	 * End the current command and all background commands
	 * Calls complete method so that everything ends properly then move to the end of the script
	 */
	void killAuto();


	virtual ~AutoManager() {  }
};


}
