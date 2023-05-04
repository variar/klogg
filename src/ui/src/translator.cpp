//
// Created by marvin on 23-5-4.
//

#include "translator.h"

#include <QApplication>
#include <exception>
#include <stack>
#include <vector>

#include "log.h"

int Translator::install( const QString& lang )
{
    if ( lang.isEmpty() ) {
        return -1;
    }

    std::array<TranslatorContext, TRANSLATOR_CNT> newTranslators{
        TranslatorContext{ QString( ":/i18n/" + lang + ".qm" ), std::make_unique<QTranslator>() },
        TranslatorContext{ QString( ":/i18n/qtbase/qtbase_" + lang + ".qm" ),
                           std::make_unique<QTranslator>() },
    };

    // remove old translators and install new translators
    std::stack<TranslatorContext*, std::vector<TranslatorContext*>> removedTranslators;
    std::stack<TranslatorContext*, std::vector<TranslatorContext*>> installedTranslators;
    try {
        for ( size_t i = 0; i < newTranslators.size(); ++i ) {
            auto& newTrans = newTranslators[ i ];
            auto& oldTrans = mTranslators_[ i ];
            // load translation file
            if ( !newTrans->load( newTrans.path ) ) {
                LOG_INFO << "load translation fail, path:" << newTrans.path;
                throw std::runtime_error( "load" );
            }
            // remove old translator
            removedTranslators.push( &oldTrans );
            QApplication::removeTranslator( oldTrans.get() );
            // install new translator
            // if lang is "en", qtbase_en.qm is empty, installation failure is expected.
            // The application uses English by default, so failure is acceptable.
            if ( !QApplication::installTranslator( newTrans.get() ) && "en" != lang ) {
                LOG_ERROR << "install translation fail,path:" << newTrans.path;
                throw std::runtime_error( "install" );
            }
            installedTranslators.push( &newTrans );
        }
    } catch ( const std::runtime_error& e ) {
        (void)e;
        // if install new translators error, recover old translators
        while ( !installedTranslators.empty() ) {
            QApplication::removeTranslator( installedTranslators.top()->get() );
            installedTranslators.pop();
        }
        while ( !removedTranslators.empty() ) {
            QApplication::installTranslator( removedTranslators.top()->get() );
            removedTranslators.pop();
        }
        return -1;
    }

    // storage installed translators
    mTranslators_.swap( newTranslators );
    return 0;
}
