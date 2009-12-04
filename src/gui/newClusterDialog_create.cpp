/*

                          Firewall Builder

                 Copyright (C) 2009 NetCitadel, LLC

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

#include "../../config.h"
#include "global.h"

#include "FWWindow.h"
#include "newClusterDialog.h"
#include "InterfacesTabWidget.h"
#include "platforms.h"
#include "FWBTree.h"
#include "FWCmdAddObject.h"

#include "fwbuilder/Cluster.h"
#include "fwbuilder/FailoverClusterGroup.h"
#include "fwbuilder/Interface.h"
#include "fwbuilder/Resources.h"
#include "fwbuilder/Policy.h"
#include "fwbuilder/NAT.h"
#include "fwbuilder/Routing.h"
#include "fwbuilder/IPv4.h"
#include "fwbuilder/IPv6.h"

#include <QtDebug>

using namespace libfwbuilder;
using namespace std;


void newClusterDialog::createNewCluster()
{
    map<int, int> id_mapping;

    QList<ClusterInterfaceData> cluster_interfaces =
        this->m_dialog->interfaceSelector->getInterfaces();
    
    typedef QPair<Firewall*, bool> fwpair;
    Firewall *master = NULL;
    QList<QPair<Firewall*, bool> > member_firewalls = 
        this->m_dialog->firewallSelector->getSelectedFirewalls();
    foreach(fwpair member, member_firewalls)
    {
        if (member.second) { master = member.first; break; }
    }

    FWObject *o;
    o = db->create(Cluster::TYPENAME);
    o->setName(string(m_dialog->obj_name->text().toUtf8().constData()));

    if (o == NULL)
    {
        QDialog::accept();
        return;
    }

    parent->add(o);

    ncl = Cluster::cast(o);

    o->setStr("platform",
              this->m_dialog->firewallSelector->getSelectedFirewalls().first().first->getStr("platform"));
    o->setStr("host_OS",
              this->m_dialog->firewallSelector->getSelectedFirewalls().first().first->getStr("host_OS"));

    foreach(EditedInterfaceData data, this->m_dialog->interfaceEditor->getNewData())
    {
        Interface *oi = Interface::cast(db->create(Interface::TYPENAME));
        oi->setName(string(data.name.toUtf8().constData()));

        ncl->add(oi);
        oi->setLabel(string(data.label.toUtf8().constData()));

        QList<QPair<Firewall*, Interface*> > member_interfaces;

        foreach(ClusterInterfaceData cid, cluster_interfaces)
        {
            if (cid.name == data.name)
            {
                member_interfaces = cid.interfaces;
                break;
            }
        }

        foreach(AddressInfo address, data.addresses)
        {
            if (address.ipv4)
            {
                QString addrname = QString("%1:%2:ip")
                               .arg(m_dialog->obj_name->text())
                               .arg(data.name);
                IPv4 *oa = IPv4::cast(db->create(IPv4::TYPENAME));
                oa->setName(string(addrname.toUtf8().constData()));
                oi->add(oa);
                oa->setAddress(InetAddr(address.address.toLatin1().constData()));
                bool ok = false ;
                int inetmask = address.netmask.toInt(&ok);
                if (ok)
                {
                    oa->setNetmask(InetAddr(inetmask));
                }
                else
                {
                    oa->setNetmask(InetAddr(address.netmask.toLatin1().constData()));
                }
            }
            else
            {
                QString addrname = QString("%1:%2:ip")
                               .arg(m_dialog->obj_name->text())
                               .arg(data.name);
                IPv6 *oa = IPv6::cast(db->create(IPv6::TYPENAME));
                oa->setName(string(addrname.toUtf8().constData()));
                oi->add(oa);
                oa->setAddress(InetAddr(AF_INET6, address.address.toLatin1().constData()));
                bool ok = false ;
                int inetmask = address.netmask.toInt(&ok);
                if (ok)
                {
                    oa->setNetmask(InetAddr(AF_INET6, inetmask));
                }
                else
                {
                    oa->setNetmask(InetAddr(AF_INET6, address.netmask.toLatin1().constData()));
                }
            }
        }

        FWOptions *ifopt;
        ifopt = oi->getOptionsObject();
        ifopt->setStr("type", "cluster_interface");

        // create failover group for this interface
        QString grpname = QString("%1:%2:members")
                          .arg(m_dialog->obj_name->text())
                          .arg(data.name);

        ClusterGroup *failover_grp = ClusterGroup::cast(
            db->create(FailoverClusterGroup::TYPENAME));
        failover_grp->setName(string(grpname.toUtf8().constData()));
        oi->add(failover_grp);

        QString failover_protocol_name = data.protocol.toLower();

        failover_grp->setStr("type",
                             failover_protocol_name.toAscii().constData());

        typedef QPair<Firewall*, Interface*> intfpair;
        foreach(intfpair intf, member_interfaces)
        {
            Firewall *member_fw = intf.first;
            Interface *member_intf = intf.second;

            id_mapping[member_intf->getId()] = oi->getId();

            failover_grp->addRef(member_intf);
            if (member_fw == master)
            {
                std::string masteriface_id =
                    FWObjectDatabase::getStringId(member_intf->getId());
                failover_grp->setStr("master_iface", masteriface_id);
            }
        }
    }

    // Copy rule sets if requested
    Firewall *source = NULL;
    foreach (QRadioButton* btn, copy_rules_from_buttons.keys())
    {
        if (btn->isChecked() && btn != this->m_dialog->noPolicy)
        {
            source = copy_rules_from_buttons[btn];
            break;
        }
    }

    if (source == NULL) return;

    FWObject *fwgroup =
        FWBTree().getStandardSlotForObject(parent->getLibrary(), Firewall::TYPENAME);

    foreach(fwpair member, member_firewalls)
    {
        Firewall *fw = member.first;
        id_mapping[fw->getId()] = ncl->getId();

        string name_bak = fw->getName() + "-bak";
        FWCmdAddObject *cmd = new FWCmdAddObject(
            mw->activeProject(), fwgroup, NULL,
            QString("Create new Firewall %1")
            .arg(QString::fromUtf8(name_bak.c_str())));
        FWObject *new_state = cmd->getNewState();
        Firewall *bakfw = Firewall::cast(new_state->addCopyOf(fw));
        bakfw->setName(name_bak);
        bakfw->setInactive(true);
        mw->activeProject()->undoStack->push(cmd);
    }

    copyRuleSets(Policy::TYPENAME, source);
    copyRuleSets(NAT::TYPENAME, source);
    copyRuleSets(Routing::TYPENAME, source);

    ncl->getRoot()->fixReferences(ncl, id_mapping);

    foreach(fwpair member, member_firewalls)
    {
        Firewall *fw = member.first;
        deleteRuleSets(Policy::TYPENAME, fw);
        deleteRuleSets(NAT::TYPENAME, fw);
        deleteRuleSets(Routing::TYPENAME, fw);
    }
}

void newClusterDialog::deleteRuleSets(const string &type, Firewall *fw)
{
    list<FWObject*> rule_sets = fw->getByType(type);
    foreach(FWObject *rs, rule_sets) fw->remove(rs);
    fw->add(db->create(type));
}

void newClusterDialog::copyRuleSets(const string &type, Firewall *source)
{
    list<FWObject*> old_ones = ncl->getByType(type);
    foreach(FWObject *old, old_ones)
        ncl->remove(old);

    FWObjectTypedChildIterator it = source->findByType(type);
    for (; it != it.end(); ++it)
    {
        ncl->addCopyOf(*it);
    }
}

