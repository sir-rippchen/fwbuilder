/*

                          Firewall Builder

                 Copyright (C) 2007 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

  $Id$

  This program is free software which we release under the GNU General Public
  License. You may redistribute and/or modify this program under the terms
  of that license as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  To get a copy of the GNU General Public License, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

//#include "config.h" <- TAPIR: wasn't commented
#include "fwbuilder_ph.h"

#include "global.h"
#include "utils_no_qt.h"

#include "fwbuilder/FWObject.h"
#include "fwbuilder/FWReference.h"
#include "fwbuilder/FWObjectDatabase.h"
#include "fwbuilder/Firewall.h"
#include "fwbuilder/Library.h"
#include "fwbuilder/Resources.h"

#if defined(_WIN32)
#  include <stdio.h>
#  include <sys/timeb.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <time.h>
#  include <windows.h>
#else
#  include <unistd.h>
#  include <string.h>
#  include <errno.h>
#  include <pwd.h>
#endif

#include <iostream>
#include <algorithm>
#include <cctype>

using namespace std;
using namespace libfwbuilder;


/**
 * this method finds all firewalls in the tree and makes no
 * assumtions about tree structure
 */
void findFirewalls(FWObject *o, std::list<FWObject*> &fwlist,
                   bool skip_system_libs)
{
    findByObjectType(o,Firewall::TYPENAME,fwlist,skip_system_libs);
}

void findHosts(FWObject *o, std::list<FWObject*> &fwlist,
               bool skip_system_libs)
{
    findByObjectType(o,Host::TYPENAME,fwlist,skip_system_libs);
}

void findByObjectType(FWObject *o,
                      const string &otype,
                      list<FWObject*> &fwlist,
                      bool skip_system_libs)
{
    if (skip_system_libs)
    {
        if (o->getId()==DELETED_LIB) return;
        if (o->getId()==TEMPLATE_LIB) return;
    }

    for (list<FWObject*>::iterator m=o->begin(); m!=o->end(); m++)
    {
        FWObject *o1=*m;
        if (FWReference::cast(o1)!=NULL) continue;
        if (o1->getTypeName()==otype) fwlist.push_back(o1);
        else                   findByObjectType(o1,otype,fwlist);
    }
}


FWReference* findRef(FWObject *o,FWObject *p)
{

    FWReference* ref=NULL;
    FWObject::iterator i=p->begin();
    for(;i!=p->end();++i)
    {
        ref=FWReference::cast(*i);
        if (ref!=NULL && ref->getPointer() == o)
        {
            break;
        }
        ref=NULL;
    }
    return ref;
}

string strip(const string &s)
{
    if (s.empty()) return s;

    string tmps = s;
    string::size_type n1,n2;

    n1 = s.find_first_not_of(" ", 0);
    n2 = s.size()-1;
    while (n2>n1 && isspace(s[n2])) n2--;
    n2++;

    return s.substr(n1, n2-n1);
}

void join::operator()(std::string &s)
{
    if (!result->empty()) *result += separator;
    *result += s;
}
