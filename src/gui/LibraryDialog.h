/* 

                          Firewall Builder

                 Copyright (C) 2003 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

  $Id: LibraryDialog.h,v 1.10 2006/05/13 06:53:05 vkurland Exp $

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


#ifndef __LIBRARYDIALOG_H_
#define __LIBRARYDIALOG_H_

#include "config.h"
#include <ui_librarydialog_q.h>
#include <QWidget>

#include "fwbuilder/FWObject.h"


class ProjectPanel;
class LibraryDialog : public QWidget
{
    Q_OBJECT

    libfwbuilder::FWObject *obj;
    bool init;
    QString  color;
    
    void fillColor();
    void changeIds(libfwbuilder::FWObject *root);
    ProjectPanel *m_project;
    
 public:
    Ui::LibraryDialog_q *m_dialog;
     
    LibraryDialog(ProjectPanel *project, QWidget *parent);
    ~LibraryDialog();
    
public slots:
    virtual void changed();
    virtual void applyChanges();
    virtual void discardChanges();
    virtual void loadFWObject(libfwbuilder::FWObject *obj);
    virtual void validate(bool*);
    virtual void isChanged(bool*);
    virtual void changeColor();
    virtual void closeEvent(QCloseEvent *e);
    
 signals:
/**
 * This signal is emitted from closeEvent, ObjectEditor connects
 * to this signal to make checks before the object editor can be closed
 * and to store its position on the screen
 */
    void close_sign(QCloseEvent *e);
    void changed_sign();
    
};

#endif // LIBRARYDIALOG_H
