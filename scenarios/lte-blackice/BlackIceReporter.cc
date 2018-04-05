#include "BlackIceReporter.h"
#include "lte_msgs/BlackIceWarning_m.h"
#include "artery/application/Middleware.h"
#include "artery/application/StoryboardSignal.h"
#include "artery/traci/VehicleController.h"
#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <omnetpp/checkandcast.h>

using namespace omnetpp;

Define_Module(BlackIceReporter)

static const simsignal_t storyboardSignal = cComponent::registerSignal("StoryboardSignal");

void BlackIceReporter::initialize()
{
    socket.setOutputGate(gate("udpOut"));
    auto centralAddress = inet::L3AddressResolver().resolve(par("centralAddress"));
    socket.connect(centralAddress, par("centralPort"));

    auto mw = inet::getModuleFromPar<artery::Middleware>(par("middlewareModule"), this);
    mw->subscribe(storyboardSignal, this);
    vehicleController = mw->getFacilities().get_const_ptr<traci::VehicleController>();
}

void BlackIceReporter::finish()
{
    socket.close();
}

void BlackIceReporter::receiveSignal(cComponent*, simsignal_t sig, cObject* obj, cObject*)
{
    if (sig == storyboardSignal) {
        auto sigobj = check_and_cast<StoryboardSignal*>(obj);
        if (sigobj->getCause() == "traction loss") {
            sendReport();
        }
    }
}

void BlackIceReporter::sendReport()
{
    Enter_Method_Silent();
    auto report = new BlackIceReport("reporting black ice");
    report->setPositionX(vehicleController->getPosition().x / boost::units::si::meter);
    report->setPositionY(vehicleController->getPosition().y / boost::units::si::meter);
    report->setSpeed(vehicleController->getSpeed() / boost::units::si::meter_per_second);
    report->setTime(simTime());
    socket.send(report);
}
