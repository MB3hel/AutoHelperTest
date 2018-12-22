#include <autonomous.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace team2655;

// Blocking command to drive
class DriveCommand: public AutoCommand{
public:
    void start(std::string commandName, std::vector<std::string> args) override {
        // Make sure there are enough arguments
        if(args.size() < 1){
            // Not enough arguments. Complete and exit function
            complete();
            return;
        }
        // Set the timeout for command based on the arguments
        setTimeout((int)(stod(args[0]) * 1000));
    }
    void process() override {
        std::cout << "Process drive." << std::endl;
    }
    void handleComplete() override {
        std::cout << "Complete drive." << std::endl;
    }
};

// Blocking command to rotate
class RotateCommand: public AutoCommand{
public:
    void start(std::string commandName, std::vector<std::string> args) override {
        // Make sure there are enough arguments
        if(args.size() < 1){
            // Not enough arguments. Complete and exit function
            complete();
            return;
        }
        // Set the timeout for command based on the arguments
        setTimeout((int)(stod(args[0]) * 1000));
    }
    void process() override {
        std::cout << "Process rotate." << std::endl;
    }
    void handleComplete() override {
        std::cout << "Complete rotate." << std::endl;
    }
};

// Background command to handle intake
class IntakeCommand : public BackgroundAutoCommand{
private:
    double speed = 0;
public:
    virtual void updateArgs(std::string commandName, std::vector<std::string> args) override {
        if(commandName == "intake_in"){
            speed = 1;
        }else if(commandName == "intake_out"){
            speed = -1;
        }else if(commandName == "intake_stop"){
            speed = 0;
            kill();
        }
    }
	virtual void process() override {
        std::cout << "Move intake " << speed << std::endl;
    }
	virtual void kill() override {
        std::cout << "Stop intake motors." << std::endl;
    }
	virtual bool shouldProcess() override {
        return speed != 0;
    }
};

// Background command to handle lifter
class LifterCommand : public BackgroundAutoCommand {
private:
    int targetPos = 0;
    int currentPos = 0;
public:
    virtual void updateArgs(std::string commandName, std::vector<std::string> args) override {
        if(args.size()  >= 1){
            targetPos = stoi(args[0]);
        }
    }
	virtual void process() override {
        if(targetPos > currentPos){
            std::cout << "raise lifter" << std::endl;
            currentPos++;
        }else{
            std::cout << "lower lifter" << std::endl;
            currentPos--;
        }
    }
	virtual void kill() override {
        targetPos = currentPos;
        std::cout << "Stop lifter" << std::endl;
    }
	virtual bool shouldProcess() override {
        return abs(targetPos - currentPos) != 0;
    }
};

AutoManager manager;

int main(){
    // Register commands with their names
    manager.registerCommand(CommandCreator<DriveCommand>, "drive");
    manager.registerCommand(CommandCreator<RotateCommand>, "rotate");
    manager.registerBackgroundCommand<IntakeCommand>({"intake_in", "intake_out", "intake_stop"});
    manager.registerBackgroundCommand<LifterCommand>("move_lifter");
    
    // Load a mock script
    manager.loadScript("F:/Projects/FRC/TestProj/Test.csv");

    // Run the mock script
    bool hasMore = true;
    do{
        hasMore = manager.process();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }while(hasMore);

    std::cout << "Simulated script complete." << std::endl;

    return 0;
}