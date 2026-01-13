// Required for most of open.mp.
#include <sdk.hpp>

// Include the pawn component information.
#include <Server/Components/Pawn/pawn.hpp>

// Include pawn-natives macros (`SCRIPT_API`) and lookups (`IPlayer&`).
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>

// Include a few function implementations.  Should only be included once.
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>


#include "SocketManager.hpp"



extern ICore* core_;

std::vector<AMX*> g_amxScripts;

extern std::unique_ptr<SocketManager> socket_manager;


class PawnSockets : public IComponent, public CoreEventHandler, public PawnEventHandler
{
private:
    
    

    IPawnComponent* pawn_ = nullptr;

public:
    
    PROVIDE_UID(0x622838F9CE56E9AF);

    // Component metadata
    StringView componentName() const override
    {
        return "PawnSockets";
    }

    SemanticVersion componentVersion() const override
    {
        return {1, 0, 0, 0};
    }


    void onAmxLoad(IPawnScript& script) override
    {
        g_amxScripts.push_back(script.GetAMX());
        pawn_natives::AmxLoad(script.GetAMX());
    }


	void onAmxUnload(IPawnScript& script) override
    {

    }

    void onInit(IComponentList* components) override { 
        pawn_ = components->queryComponent<IPawnComponent>();
        core_->getEventDispatcher().addEventHandler(this);
        if(pawn_)
        {
            setAmxFunctions(pawn_->getAmxFunctions());
            setAmxLookups(components);
            pawn_->getEventDispatcher().addEventHandler(this);
        }
    }

    // Called when the component is loaded
    void onLoad(ICore* core) override
    {
        
        core_ = core;
        core_->printLn("PawnSockets loaded.");
        socket_manager = std::make_unique<SocketManager>();
    }


    // Called after all components are loaded
    void onReady() override
    {
        core_->printLn("PawnSockets ready.");
        socket_manager->start(core_);
    }

    void onFree(IComponent* component) override {
        if(component == pawn_){
            pawn_ = nullptr;
            setAmxFunctions();
            setAmxLookups();
        }
    }

    // Called when the server shuts down or component unloads
    void free() override
    {
        socket_manager->stop();
        delete this;
    }
    
    void reset() override { };

    void onTick(Microseconds elapsed, TimePoint now) override {
        if(socket_manager != nullptr){
            if(socket_manager->getState() == true){
                socket_manager->dispatch();
            }
        }
    }
};



COMPONENT_ENTRY_POINT()
{
	return new PawnSockets();
}

