//
// Created by marvin on 23-5-4.
//

#ifndef KLOGG_TRANSLATOR_H
#define KLOGG_TRANSLATOR_H

#include <QString>
#include <QTranslator>
#include <array>
#include <memory>

class Translator {
  public:
    Translator() = default;
    ~Translator() = default;
    Translator( Translator& ) = delete;
    Translator( Translator&& ) = delete;
    Translator operator=( Translator& ) = delete;
    Translator operator=( Translator&& ) = delete;

    int install( const QString& lang );

  private:
    enum { TRANSLATOR_CNT = 2 };

    struct TranslatorContext {
        QString path;
        std::unique_ptr<QTranslator> translator;

        inline std::unique_ptr<QTranslator>& operator->()
        {
            return translator;
        }
        inline QTranslator* get()
        {
            return translator.get();
        }
    };

    std::array<TranslatorContext, TRANSLATOR_CNT> mTranslators_;
};

#endif // KLOGG_TRANSLATOR_H
