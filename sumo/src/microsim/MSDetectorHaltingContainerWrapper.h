#ifndef MSDETECTORHALTINGCONTAINERWRAPPER_H
#define MSDETECTORHALTINGCONTAINERWRAPPER_H

/**
 * @file   MSDetectorHaltingContainerWrapper.h
 * @author Christian Roessel
 * @date   Started Fri Sep 26 19:11:26 2003
 * @version $Id$
 * @brief
 *
 *
 */

/* Copyright (C) 2003 by German Aerospace Center (http://www.dlr.de) */

//---------------------------------------------------------------------------//
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//---------------------------------------------------------------------------//


#include "MSDetectorContainerWrapper.h"
#include "MSDetectorOccupancyCorrection.h"
#include "MSUnit.h"
#include "MSUpdateEachTimestep.h"
#include <list>
#include <map>

class MSVehicle;

template< class WrappedContainer >
struct MSDetectorHaltingContainerWrapper :
    public MSDetectorContainerWrapper< WrappedContainer >,
    public MSUpdateEachTimestep<
    MSDetectorHaltingContainerWrapper< WrappedContainer > >
{
    typedef typename WrappedContainer::iterator HaltingsIt;
    typedef typename WrappedContainer::const_iterator HaltingsConstIt;
    typedef WrappedContainer InnerContainer;

    MSDetectorHaltingContainerWrapper(
        MSUnit::Steps timeThreshold,
        MSUnit::CellsPerStep speedThreshold,
        MSUnit::Cells jamDistThreshold )
        : MSDetectorContainerWrapper< WrappedContainer >(),
          MSUpdateEachTimestep<
          MSDetectorHaltingContainerWrapper< WrappedContainer > >(),
          timeThresholdM( timeThreshold ),
          speedThresholdM( speedThreshold ),
          jamDistThresholdM( jamDistThreshold )
        {}
    
    MSDetectorHaltingContainerWrapper(
        const MSDetectorOccupancyCorrection& occupancyCorrection,
        MSUnit::Steps timeThreshold,
        MSUnit::CellsPerStep speedThreshold,
        MSUnit::Cells jamDistThreshold )
        : MSDetectorContainerWrapper< WrappedContainer >( occupancyCorrection),
          MSUpdateEachTimestep<
          MSDetectorHaltingContainerWrapper< WrappedContainer > >(),
          timeThresholdM( timeThreshold ),
          speedThresholdM( speedThreshold ),
          jamDistThresholdM( jamDistThreshold )
        {}

    bool updateEachTimestep( void )
        {
            // set posM, isHaltingM and haltingDurationM
            for ( HaltingsIt haltIt = containerM.begin();
                  haltIt != containerM.end(); ++haltIt ) {
                haltIt->posM += haltIt->vehM->getMovedDistance();
                if ( haltIt->vehM->speed() >= speedThresholdM ) {
                    haltIt->timeBelowSpeedThresholdM = 0;
                    haltIt->isHaltingM = false;
                    haltIt->haltingDurationM = 0.0;
                }
                else {
                    haltIt->timeBelowSpeedThresholdM++;
                    if ( haltIt->timeBelowSpeedThresholdM > timeThresholdM ) {
                        if ( ! haltIt->isHaltingM ) {
                            haltIt->isHaltingM = true;
                            // time to detect halting contributes to
                            // halting-duration
                            haltIt->haltingDurationM =
                                haltIt->timeBelowSpeedThresholdM++;
                        }
                        else {
                            haltIt->haltingDurationM++;
                        }
                    }
                }
            }

            // set isInJamM
            if ( containerM.size() >= 1 ) {
                containerM.begin()->isInJamM = false;
            }
            if ( containerM.size() > 1 ) {
                for ( HaltingsIt jamIt = containerM.begin();
                      jamIt != --containerM.end(); /* empty */ ){
                    HaltingsIt rearIt = jamIt;
                    HaltingsIt frontIt = ++jamIt;
                    if ( ! rearIt->isHaltingM || ! frontIt->isHaltingM ) {
                        continue;
                    }
                    if ( frontIt->posM - frontIt->vehM->length()
                         - rearIt->posM <= jamDistThresholdM ) {
                        if ( rearIt == containerM.begin() ) {
                            rearIt->isInJamM = true;
                        }
                        frontIt->isInJamM = true;
                    }
                    else {
                        frontIt->isInJamM = false;
                    }
                }
            }
            return false;
        }

    MSUnit::Steps timeThresholdM;
    MSUnit::CellsPerStep speedThresholdM;
    MSUnit::Cells jamDistThresholdM;
};

// specialization for WrappedContainer == map< MSVehicle*, T >
template< class T >
struct MSDetectorHaltingContainerWrapper< std::map< MSVehicle*, T > > :
    public MSDetectorContainerWrapper< std::map< MSVehicle*, T > >,
    public MSUpdateEachTimestep<
    MSDetectorHaltingContainerWrapper< std::map< MSVehicle*, T > > >
{
    typedef std::map< MSVehicle*, T > WrappedContainer;
    typedef typename WrappedContainer::iterator HaltingsIt;
    typedef typename WrappedContainer::const_iterator HaltingsConstIt;
    typedef WrappedContainer InnerContainer;

    MSDetectorHaltingContainerWrapper(
        MSUnit::Steps timeThreshold,
        MSUnit::CellsPerStep speedThreshold
        )
        :
        MSDetectorContainerWrapper< WrappedContainer >(),
        MSUpdateEachTimestep<
        MSDetectorHaltingContainerWrapper< WrappedContainer > >(),
        timeThresholdM( timeThreshold ),
        speedThresholdM( speedThreshold )
        {}
    
    MSDetectorHaltingContainerWrapper(
        const MSDetectorOccupancyCorrection& occupancyCorrection,
        MSUnit::Steps timeThreshold,
        MSUnit::CellsPerStep speedThreshold
        )
        :
        MSDetectorContainerWrapper< WrappedContainer >( occupancyCorrection),
        MSUpdateEachTimestep<
        MSDetectorHaltingContainerWrapper< WrappedContainer > >(),
        timeThresholdM( timeThreshold ),
        speedThresholdM( speedThreshold )
        {}

    bool updateEachTimestep( void )
        {
            for ( HaltingsIt pair = containerM.begin();
                  haltIt != containerM.end(); ++haltIt ) {
                MSVehicle* veh = pair->first;
                E3Halting& halt = pair->second;
                halt.posM += veh->getMovedDistance();
                if ( veh->speed() >= speedThresholdM ) {
                    halt.timeBelowSpeedThresholdM = 0;
                    halt.isHaltingM = false;
                    halt.haltingDurationM = 0.0;
                }
                else {
                    halt.timeBelowSpeedThresholdM++;
                    if ( halt.timeBelowSpeedThresholdM > timeThresholdM ) {
                        if ( ! halt.isHaltingM ) {
                            // beginning of new halt detected
                            halt.isHaltingM = true;
                            // time to detect halting contributes to
                            // halting-duration
                            halt.haltingDurationM =
                                halt.timeBelowSpeedThresholdM++;
                            halt.nHalts++;
                        }
                        else {
                            halt.haltingDurationM++;
                        }
                    }
                }
            }
            return false; // to please MSVC++
        }

    MSUnit::Steps timeThresholdM;
    MSUnit::CellsPerStep speedThresholdM;
    MSUnit::Cells jamDistThresholdM;
};




namespace DetectorContainer
{
    struct Halting
    {
        Halting( MSVehicle* veh )
            : vehM( veh ),
              posM( vehM->pos() ),
              timeBelowSpeedThresholdM( 0 ),
              isHaltingM( false ),
              isInJamM( false ),
              haltingDurationM( 0 )
            {}
        MSVehicle* vehM;
        MSUnit::Cells posM; 
        MSUnit::Steps timeBelowSpeedThresholdM;
        bool isHaltingM;
        bool isInJamM;
        MSUnit::Steps haltingDurationM;
    };

    struct E3Halting
    {
        E3Halting( MSVehicle* veh )
            :
            posM( veh->pos() )
            , timeBelowSpeedThresholdM( 0 )
            , isHaltingM( false )
            , haltingDurationM( 0 )
            , nHalts( 0 )
            {}
        MSUnit::Cells posM; 
        MSUnit::Steps timeBelowSpeedThresholdM;
        bool isHaltingM;
        MSUnit::Steps haltingDurationM;
        unsigned nHalts;
    };    

    typedef MSDetectorHaltingContainerWrapper<
        std::list< Halting > > HaltingsList;

    typedef MSUpdateEachTimestep< HaltingsList > UpdateHaltings;

    typedef MSDetectorHaltingContainerWrapper<
        std::map< MSVehicle*, E3Halting > > HaltingsMap;

    typedef MSUpdateEachTimestep< HaltingsMap > UpdateE3Haltings;   
}


namespace Predicate
{
	// specialization
    template<>
    struct PosGreaterC< DetectorContainer::Halting > :
        public std::binary_function< DetectorContainer::Halting,
                                     double, bool >
    {
        bool operator() ( const DetectorContainer::Halting& item,
                          double pos ) const {
            return item.posM > pos;
        }
    };

    // specialization
    template<>
    struct VehEqualsC< DetectorContainer::Halting > :
        public std::binary_function< DetectorContainer::Halting,
                                     MSVehicle*, bool >
    {
        bool operator() ( DetectorContainer::Halting item,
                          const MSVehicle* veh ) const {
            return item.vehM == veh;
        }
    };
}


#endif // MSDETECTORHALTINGCONTAINER_H

// Local Variables:
// mode:C++
// End:
