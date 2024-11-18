/*
 * Copyright (C) 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MESSAGERECEIVER_H
#define MESSAGERECEIVER_H

#include <QtCore/QCborValue>

#include <QtCore/QJsonDocument>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "log.h"
#include "klogg_version.h"

/*
 * Class receiving messages from another instance of klogg.
 * Messages are forwarded to the application by signals.
 */
class MessageReceiver final : public QObject {
    Q_OBJECT

  public:
    MessageReceiver()
        : QObject()
    {
    }

  Q_SIGNALS:
    void loadFile( const QString& filename );

  public Q_SLOTS:
    void receiveMessage( const QByteArray& message )
    {
        const auto data = QCborValue::fromCbor( message ).toVariant().toMap();

        LOG_INFO << "Message " << QJsonDocument::fromVariant(data).toJson();

        if ( data[ "version" ].toString() != kloggVersion() ) {
            return;
        }

        QStringList filenames = data[ "files" ].toStringList();

        for ( const auto& f : filenames ) {
            Q_EMIT loadFile( f );
        }
    }
};

#endif // MESSAGERECEIVER_H
