/*

                          Firewall Builder

                 Copyright (C) 2003 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

  $Id: HostDialog.cpp,v 1.20 2007/04/14 00:18:43 vkurland Exp $

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

#include "fwbuilder_ph.h"

#include "config.h"
#include "global.h"
#include "utils.h"

#include "FWBTree.h"
#include "HostDialog.h"
#include "ProjectPanel.h"

#include "fwbuilder/Library.h"
#include "fwbuilder/Host.h"
#include "fwbuilder/Interface.h"
#include "fwbuilder/Management.h"
#include "fwbuilder/FWException.h"

#include <qlineedit.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qtextedit.h>
#include <qcombobox.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include "FWBSettings.h"

#include "FWWindow.h"

using namespace std;
using namespace libfwbuilder;

HostDialog::HostDialog(ProjectPanel *project, QWidget *parent) : QWidget(parent), m_project(project)
{
    m_dialog = new Ui::HostDialog_q;
    m_dialog->setupUi(this);
    setFont(st->getUiFont());
    obj=NULL;
}

HostDialog::~HostDialog()
{
    delete m_dialog;
}

void HostDialog::loadFWObject(FWObject *o)
{
    obj=o;
    Host *s = dynamic_cast<Host*>(obj);
    assert(s!=NULL);

    init=true;

    fillLibraries(m_dialog->libs,obj);

    Management *mgmt=s->getManagementObject();
    assert(mgmt!=NULL);

    FWOptions  *opt =s->getOptionsObject();

    m_dialog->obj_name->setText( QString::fromUtf8(s->getName().c_str()) );
//    snmpCommunity->setText( mgmt->getSNMPManagement()->getReadCommunity().c_str() );
    m_dialog->MACmatching->setChecked( opt->getBool("use_mac_addr_filter") );

    m_dialog->comment->setText( QString::fromUtf8(s->getComment().c_str()) );

    //apply->setEnabled( false );

    m_dialog->obj_name->setEnabled(!o->isReadOnly());
    setDisabledPalette(m_dialog->obj_name);

    m_dialog->libs->setEnabled(!o->isReadOnly());
    setDisabledPalette(m_dialog->libs);

//    snmpCommunity->setEnabled(!o->isReadOnly());
//    setDisabledPalette(snmpCommunity);

    m_dialog->MACmatching->setEnabled(!o->isReadOnly());
    setDisabledPalette(m_dialog->MACmatching);

    m_dialog->comment->setReadOnly(o->isReadOnly());
    setDisabledPalette(m_dialog->comment);


    init=false;
}

void HostDialog::changed()
{
    //apply->setEnabled( true );
    emit changed_sign();
}

void HostDialog::validate(bool *res)
{
    *res=true;
    if (!isTreeReadWrite(this,obj)) { *res=false; return; }
    if (!validateName(this,obj,m_dialog->obj_name->text())) { *res=false; return; }
}

void HostDialog::isChanged(bool *res)
{
    //*res=(!init && apply->isEnabled());
}

void HostDialog::libChanged()
{
    changed();
}

void HostDialog::applyChanges()
{
    Host *s = dynamic_cast<Host*>(obj);
    assert(s!=NULL);

    Management *mgmt=s->getManagementObject();
    assert(mgmt!=NULL);

    FWOptions  *opt =s->getOptionsObject();

    string oldname=obj->getName();
    obj->setName( string(m_dialog->obj_name->text().toUtf8().constData()) );
    obj->setComment( string(m_dialog->comment->toPlainText().toUtf8().constData()) );
//    mgmt->getSNMPManagement()->setReadCommunity( snmpCommunity->text().latin1() );
    opt->setBool("use_mac_addr_filter", m_dialog->MACmatching->isChecked() );

    mw->updateObjName(obj,QString::fromUtf8(oldname.c_str()));

    init=true;

/* move to another lib if we have to */
    if (! m_project->isSystem(obj) &&
	m_dialog->libs->currentText() != QString(obj->getLibrary()->getName().c_str()))
        mw->moveObject(m_dialog->libs->currentText(), obj);

    init=false;

    //apply->setEnabled( false );
    mw->updateLastModifiedTimestampForAllFirewalls(obj);
}

void HostDialog::discardChanges()
{
    loadFWObject(obj);
}


/* ObjectEditor class connects its slot to this signal and does all
 * the verification for us, then accepts (or not) the event. So we do
 * nothing here and defer all the processing to ObjectEditor
 */
void HostDialog::closeEvent(QCloseEvent *e)
{
    emit close_sign(e);

}

