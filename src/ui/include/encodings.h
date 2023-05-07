//
// Created by omni on 16.12.2019.
//

#ifndef KLOGG_ENCODINGS_H
#define KLOGG_ENCODINGS_H

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // Q_OS_WIN

#include <QActionGroup>
#include <QApplication>
#include <QMenu>
#include <QString>
#include <QTextCodec>

#include <configuration.h>
#include <mainwindowtext.h>

class EncodingMenu {
  public:
    static std::map<QString, std::vector<int>> supportedEncodings()
    {
        return std::map<QString, std::vector<int>>{
            { "Unicode", { 106, 1013, 1014, 1018, 1019 } },
            { "Arabic", { 82, 2256 } },
            { "Baltic", { 7, 109, 2257 } },
            { "Celtic", { 110 } },
            { "Cyrillic", { 8, 2084, 2088, 2027, 2086, 2251 } },
            { "Central European", { 2250 } },
            { "Chinese", { 2026, 2025 } },
            { "Eastern European", { 5 } },
            { "Greek", { 10, 2253 } },
            { "Hebrew", { 85, 2255 } },
            { "Japanese", { 17, 18, 39 } },
            { "Korean", { -949, 38 } },
            { "Thai", { 2259 } },
            { "Turkish", { 6, 12, 2254 } },
            { "Western European", { 3, 4, 111, 2009, 2252 } },
            { "Vietnamese", { 2258 } }
        };
    }

    static QMenu* generate( QActionGroup* actionGroup )
    {
        const auto supportedEncodings = EncodingMenu::supportedEncodings();
        const auto defaultEncodingMib = Configuration::get().defaultEncodingMib();

        using namespace klogg::mainwindow;
        QMenu* encodingsMenu = new QMenu(
            QApplication::translate( "klogg::mainwindow::menu", menu::encodingTitle ) );

        auto autoEncoding = encodingsMenu->addAction(
            QApplication::translate( "klogg::mainwindow::action", action::autoEncodingText ) );
        autoEncoding->setStatusTip(
            QApplication::translate( "klogg::mainwindow::action", action::autoEncodingStatusTip ) );
        autoEncoding->setCheckable( true );
        autoEncoding->setActionGroup( actionGroup );
        autoEncoding->setChecked( defaultEncodingMib < 0 );

        encodingsMenu->addSeparator();

        QTextCodec* systemCodec = nullptr;

#ifdef Q_OS_WIN
        auto systemCodePage = ::GetACP();
        systemCodec
            = QTextCodec::codecForName( QString( "CP%1" ).arg( systemCodePage ).toLatin1() );
#endif
        if ( systemCodec == nullptr ) {
            systemCodec = QTextCodec::codecForLocale();
        }

        auto systemEncodingName
            = QApplication::tr( "System (%1)" ).arg( systemCodec->name().constData() );

        auto systemEncoding = encodingsMenu->addAction( systemEncodingName );
        systemEncoding->setCheckable( true );
        systemEncoding->setActionGroup( actionGroup );
        systemEncoding->setData( systemCodec->mibEnum() );

        encodingsMenu->addSeparator();

        for ( const auto& group : supportedEncodings ) {
            auto menu = encodingsMenu->addMenu( group.first );
            for ( const auto mib : group.second ) {
                auto codec = QTextCodec::codecForMib( mib );
                if ( codec ) {
                    auto action = menu->addAction( QString::fromLatin1( codec->name() ) );
                    action->setActionGroup( actionGroup );
                    action->setData( mib );
                    action->setCheckable( true );
                    action->setChecked( defaultEncodingMib == mib );
                }
            }
        }

        return encodingsMenu;
    }
};

#endif // KLOGG_ENCODINGS_H
