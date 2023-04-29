/*
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.
 
 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//  Created by Boris Vigman.
//  Copyright © 2019-2023 Boris Vigman. All rights reserved.

#include "CPPAFKBase.hpp"

namespace AsyncFlowKit {
    CPPAFKCompositionPar::CPPAFKCompositionPar()
    {
    };
    CPPAFKCompositionPar::~CPPAFKCompositionPar()
    {
    };
#ifdef CPPAFK_USE_STR_OBJ_NAME
    CPPAFKNumId CPPAFKCompositionPar::createSession(CPPAFKSessionConfigParams* separams, std::string& sid){
        CPPAFKNumId numid;
        
        return numid;
    }
#endif
    CPPAFKNumId CPPAFKCompositionPar::createSession(CPPAFKSessionConfigParams* exparams){
        CPPAFKNumId numid;
        //decode params
        eCPPAFKBlockingCallMode blkMode=AFK_BC_NO_BLOCK;
        bool dataMode=true;
        //create session
        CPPAFKComposingSession* se=new CPPAFKComposingSession(blkMode, dataMode);

        
        return numid;
    }
    
    bool CPPAFKCompositionPar::_decodeParams(CPPAFKSessionConfigParams* params){
        return true;
    }
    bool CPPAFKCompositionPar::cast(CPPAFKBasicDataObjectWrap, CPPAFKExecutionParams*){
        return true;
    }
    bool CPPAFKCompositionPar::call(CPPAFKBasicDataObjectWrap, CPPAFKExecutionParams*){
        return true;
    }
    
}

