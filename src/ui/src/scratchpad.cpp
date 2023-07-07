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

#include "scratchpad.h"

#include <memory>
#include <optional>

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDomDocument>
#include <QFormLayout>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QTimeZone>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>

#include "crc32.h"

namespace klogg {

class DateTimeBox : public QFormLayout {
  public:
    DateTimeBox();
    ~DateTimeBox() = default;

    QString displayTime( const QString& text );

  private:
    QString displayTime();

  private:
    std::optional<qint64> timestamp_;
    QLineEdit* timeLine_;
    QComboBox* tzComboBox_;
};
} // namespace klogg

namespace {

template <typename T>
QString formatHex( T value )
{
    return QString::fromLatin1( QByteArray::number( value, 16 ).rightJustified( 8, '0', false ) );
}

template <typename T>
QString formatDec( T value )
{
    return QString::fromLatin1( QByteArray::number( value, 10 ) );
}

constexpr int StatusTimeout = 2000;

constexpr qint64 FileTimeTicks = 10000000;
constexpr qint64 SecondsToEpoch = 11644473600LL;

qint64 windowsTickToUnixSeconds( qint64 windowsTicks )
{
    return ( windowsTicks / FileTimeTicks - SecondsToEpoch );
}

} // namespace

ScratchPad::ScratchPad( QWidget* parent )
    : QWidget( parent )
{
    this->hide();
    auto textEdit = std::make_unique<QPlainTextEdit>();
    textEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    textEdit->setMinimumSize( 300, 300 );
    textEdit->setUndoRedoEnabled( true );

    auto toolBar = std::make_unique<QToolBar>();

    auto decodeBase64Action = std::make_unique<QAction>( "From base64" );
    connect( decodeBase64Action.get(), &QAction::triggered, [ this ]( auto ) { decodeBase64(); } );
    toolBar->addAction( decodeBase64Action.release() );

    auto encodeBase64Action = std::make_unique<QAction>( "To base64" );
    connect( encodeBase64Action.get(), &QAction::triggered, [ this ]( auto ) { encodeBase64(); } );
    toolBar->addAction( encodeBase64Action.release() );

    auto decodeHexAction = std::make_unique<QAction>( "From hex" );
    connect( decodeHexAction.get(), &QAction::triggered, [ this ]( auto ) { decodeHex(); } );
    toolBar->addAction( decodeHexAction.release() );

    auto encodeHexAction = std::make_unique<QAction>( "To hex" );
    connect( encodeHexAction.get(), &QAction::triggered, [ this ]( auto ) { encodeHex(); } );
    toolBar->addAction( encodeHexAction.release() );

    auto decodeUrlAction = std::make_unique<QAction>( "Decode url" );
    connect( decodeUrlAction.get(), &QAction::triggered, [ this ]( auto ) { decodeUrl(); } );
    toolBar->addAction( decodeUrlAction.release() );

    toolBar->addSeparator();

    auto formatJsonAction = std::make_unique<QAction>( "Format json" );
    connect( formatJsonAction.get(), &QAction::triggered, [ this ]( auto ) { formatJson(); } );
    toolBar->addAction( formatJsonAction.release() );

    auto formatXmlAction = std::make_unique<QAction>( "Format xml" );
    connect( formatXmlAction.get(), &QAction::triggered, [ this ]( auto ) { formatXml(); } );
    toolBar->addAction( formatXmlAction.release() );

    toolBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );

    auto statusBar = std::make_unique<QStatusBar>();

    auto transLayout = std::make_unique<QFormLayout>();

    auto addBoxToLayout
        = [ &transLayout, this ]( const QString& label, QLineEdit** widget, auto changeFunction ) {
              auto box = std::make_unique<QLineEdit>();
              box->setReadOnly( true );
              *widget = box.get();
              transLayout->addRow( label, box.release() );

              connect( this, &ScratchPad::updateTransformation, this, changeFunction );
          };

    addBoxToLayout( "CRC32 hex", &crc32HexBox_, &ScratchPad::crc32Hex );
    addBoxToLayout( "CRC32 dec", &crc32DecBox_, &ScratchPad::crc32Dec );
    addBoxToLayout( "File time", &fileTimeBox_, &ScratchPad::fileTime );
    addBoxToLayout( "Dec->Hex", &decToHexBox_, &ScratchPad::decToHex );
    addBoxToLayout( "Hex->Dec", &hexToDecBox_, &ScratchPad::hexToDec );
    timeBox_ = new klogg::DateTimeBox();
    transLayout->addRow( timeBox_ );
    connect( this, &ScratchPad::updateTransformation, [ this ]() {
        transformText( [ this ]( QString text ) { return timeBox_->displayTime( text ); } );
    } );

    auto hLayout = std::make_unique<QHBoxLayout>();
    hLayout->addWidget( textEdit.get(), 3 );
    hLayout->addLayout( transLayout.release(), 2 );

    auto vLayout = std::make_unique<QVBoxLayout>();
    vLayout->addWidget( toolBar.release() );
    vLayout->addLayout( hLayout.release() );
    vLayout->addWidget( statusBar.get() );

    textEdit_ = textEdit.release();
    statusBar_ = statusBar.release();

    this->setLayout( vLayout.release() );

    connect( textEdit_, &QPlainTextEdit::textChanged, this, &ScratchPad::updateTransformation );
    connect( textEdit_, &QPlainTextEdit::selectionChanged, this,
             &ScratchPad::updateTransformation );
}

void ScratchPad::addData( QString newData )
{
    if ( newData.isEmpty() ) {
        return;
    }

    textEdit_->appendPlainText( newData );
}

void ScratchPad::replaceData( QString newData )
{
    if ( newData.isEmpty() ) {
        return;
    }

    textEdit_->setPlainText( newData );
}

QString ScratchPad::transformText( const std::function<QString( QString )>& transform )
{
    auto cursor = textEdit_->textCursor();
    auto text = cursor.selectedText();
    if ( text.isEmpty() ) {
        cursor.select( QTextCursor::Document );
        text = cursor.selectedText();
    }

    return transform( text );
}

void ScratchPad::transformTextInPlace( const std::function<QString( QString )>& transform )
{
    auto cursor = textEdit_->textCursor();
    auto text = cursor.selectedText();
    if ( text.isEmpty() ) {
        cursor.select( QTextCursor::Document );
        text = cursor.selectedText();
    }

    auto transformedText = transform( text );

    if ( !transformedText.isEmpty() ) {
        cursor.insertText( transformedText );
        textEdit_->setTextCursor( cursor );

        QApplication::clipboard()->setText( transformedText );

        statusBar_->showMessage( "Copied to clipboard", StatusTimeout );
    }
    else {
        statusBar_->showMessage( "Empty transformation", StatusTimeout );
    }
}

void ScratchPad::decodeUrl()
{
    transformTextInPlace(
        []( QString text ) { return QUrl::fromPercentEncoding( text.toUtf8() ); } );
}

void ScratchPad::decodeBase64()
{
    transformTextInPlace( []( QString text ) {
        auto decoded = QByteArray::fromBase64( text.toUtf8() );
        return QString::fromStdString( { decoded.begin(), decoded.end() } );
    } );
}

void ScratchPad::encodeBase64()
{
    transformTextInPlace( []( QString text ) {
        auto encoded = text.toUtf8().toBase64();
        return QString::fromLatin1( encoded );
    } );
}

void ScratchPad::decodeHex()
{
    transformTextInPlace( []( QString text ) {
        auto decoded = QByteArray::fromHex( text.toUtf8() );
        return QString::fromStdString( { decoded.begin(), decoded.end() } );
    } );
}

void ScratchPad::encodeHex()
{
    transformTextInPlace( []( QString text ) {
        auto encoded = text.toUtf8().toHex();
        return QString::fromLatin1( encoded );
    } );
}

void ScratchPad::crc32Hex()
{
    crc32HexBox_->setText( transformText( []( QString text ) {
        const auto decoded = Crc32::calculate( text.toUtf8() );
        return formatHex( decoded ).prepend( "0x" );
    } ) );
}

void ScratchPad::crc32Dec()
{
    crc32DecBox_->setText( transformText( []( QString text ) {
        const auto decoded = Crc32::calculate( text.toUtf8() );
        return formatDec( decoded );
    } ) );
}

void ScratchPad::fileTime()
{
    fileTimeBox_->setText( transformText( []( QString text ) {
        bool isOk = false;
        const auto time = text.toUtf8().toLongLong( &isOk );
        if ( isOk ) {
            QDateTime dateTime;
            dateTime.setTimeSpec( Qt::UTC );
            dateTime.setSecsSinceEpoch( windowsTickToUnixSeconds( time ) );
            return dateTime.toString( Qt::ISODate );
        }
        else {
            return QString{};
        }
    } ) );
}

void ScratchPad::decToHex()
{
    decToHexBox_->setText( transformText( []( QString text ) {
        bool isOk = false;
        const auto value = text.toUtf8().toLongLong( &isOk );
        if ( isOk ) {
            return formatHex( value );
        }
        else {
            return QString{};
        }
    } ) );
}

void ScratchPad::hexToDec()
{
    hexToDecBox_->setText( transformText( []( QString text ) {
        bool isOk = false;
        const auto value = text.toUtf8().toLongLong( &isOk, 16 );
        if ( isOk ) {
            return formatDec( value );
        }
        else {
            return QString{};
        }
    } ) );
}

void ScratchPad::formatJson()
{
    transformTextInPlace( []( QString text ) {
        const auto start = std::min( text.indexOf( '{' ), text.indexOf( '[' ) );

        QJsonParseError parseError;
        auto json = QJsonDocument::fromJson( text.mid( start ).toUtf8(), &parseError );
        if ( json.isNull() ) {
            json = QJsonDocument::fromJson( text.mid( start, parseError.offset ).toUtf8(),
                                            &parseError );
        }

        return json.toJson( QJsonDocument::Indented );
    } );
}

void ScratchPad::formatXml()
{
    transformTextInPlace( []( QString text ) {
        const auto start = text.indexOf( '<' );

        QDomDocument xml;
        xml.setContent( text.mid( start ).toUtf8() );

        return xml.toString( 2 );
    } );
}

klogg::DateTimeBox::DateTimeBox()
    : QFormLayout()
    , timestamp_()
    , timeLine_( new QLineEdit() )
    , tzComboBox_( new QComboBox() )
{
    addRow( tr( "Time" ), timeLine_ );
    timeLine_->setReadOnly( true );

    addRow( tr( "TimeZone" ), tzComboBox_ );
    connect( tzComboBox_, &QComboBox::currentTextChanged, [ this ] { displayTime(); } );
    auto ids = QTimeZone::availableTimeZoneIds();
    std::for_each( ids.begin(), ids.end(), [ & ]( const auto& item ) {
        if ( item.contains( "UTC" ) ) {
            tzComboBox_->addItem( item );
        }
    } );
}

QString klogg::DateTimeBox::displayTime( const QString& text )
{
    bool isOk = false;
    const auto unixTime = text.toUtf8().toLongLong( &isOk );
    if ( !isOk ) {
        timestamp_.reset();
        timeLine_->setText( QString{} );
        return QString{};
    }
    timestamp_ = unixTime;
    return displayTime();
}

QString klogg::DateTimeBox::displayTime()
{
    if ( !timestamp_ ) {
        return QString{};
    }

    // Convert to a date string for the selected time zone
    auto tz = QTimeZone( tzComboBox_->currentData( Qt::DisplayRole ).toByteArray() );
    auto dateTime = QDateTime::fromSecsSinceEpoch( timestamp_.value() ).toTimeZone( tz );
    timeLine_->setText( dateTime.toString( Qt::ISODate ) );
    timeLine_->setCursorPosition( 0 );

    return timeLine_->text();
}
