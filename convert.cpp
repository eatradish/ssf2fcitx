//
// Created by void001 on 9/29/18.
// Converts the extracted ssf theme to fcitx theme

#include <QSettings>
#include <QDir>
#include <iostream>
#include <codecvt>
#include <locale>
#include <string>
#include <iconv.h>
#include <QProcess>
#include <wait.h>
#include <unistd.h>
#include <QVector>
#include <QTextCodec>
#include <QtGui/QPixmap>

#define MAX_CHARS 255

const double pt2px_scale = 1.333;

char convert_from_keys[][MAX_CHARS] = {
        "General/zhongwen_first_color",
};

char convert_to_keys[][MAX_CHARS] = {

};


int do_convert(char *skindir) {
    /*
     * First convert config file to UTF8 encoding
     */

    // TODO: remove the iconv call
    QDir dir(skindir);
    QProcess process;
    QStringList args;
    args.append("--from-code=UTF16");
    args.append("--to-code=UTF8");
    args.append(dir.filePath("skin.ini"));
    args.append("--output=" + dir.filePath("skin.ini.u8"));
    qint64 pid;

    process.startDetached("iconv", args,  NULL, &pid);
    usleep(10);

    QSettings ssfconf(dir.filePath("skin.ini.u8"), QSettings::IniFormat);
    ssfconf.setIniCodec("UTF-8");
    QSettings fcitxconf(dir.filePath("fcitx_skin.conf"), QSettings::NativeFormat);
    fcitxconf.setIniCodec("UTF-8");
    fcitxconf.clear();
    QStringList list = ssfconf.allKeys();

    std::cout << dir.filePath("skin.ini").toStdString() << std::endl;
    const char *fileName = dir.filePath("skin.ini").toStdString().c_str();

    if(process.exitCode() != 0) {
        std::cout << "Exit code not zero, code = " << process.exitCode() << std::endl;
    }

    for(auto x : list) {
        auto key = x.toStdWString();
        std::wcout << key << std::endl;
    }

    fcitxconf.beginGroup("SkinInfo");
    fcitxconf.setValue("Name", "[CONVERT FROM SSF]" + ssfconf.value("skin_name").toString());
    fcitxconf.setValue("Version", ssfconf.value("skin_version").toString());
    fcitxconf.setValue("Author", ssfconf.value("skin_author").toString() + "<" + ssfconf.value("skin_email").toString() + ">");
    fcitxconf.setValue("Desc", ssfconf.value("skin_info").toString());
    fcitxconf.endGroup();

    ssfconf.beginGroup("Display");
    fcitxconf.beginGroup("SkinFont");
    fcitxconf.setValue("FontSize", ssfconf.value("font_size").toInt());
    // we need to add font_pix to the offset
    int font_pix = (int)(ssfconf.value("font_size").toInt() * pt2px_scale);
    fcitxconf.setValue("MenuFontSize", ssfconf.value("font_size").toInt() - 3);
    fcitxconf.setValue("RespectDPI", true);

    // zhongwen_first_color => FirstCandColor
    auto val = ssfconf.value("zhongwen_first_color");
    uint32_t ptr = val.toString().toInt(nullptr, 16);
    // printf("%X", ptr);
    char buf[MAX_CHARS];
    sprintf(buf, "%d %d %d", (ptr & 0xFF0000) >> 16, (ptr & 0x00FF00) >> 8, ptr & 0x0000FF);
    // fcitxconf.setValue("FirstCandColor", QString(buf));

    // zhongwen_first_color => UserPhraseColor
    val = ssfconf.value("pinyin_color");
    ptr = val.toString().toInt(nullptr, 16);
    // PSST: sogou is fxxking your brain with out of order RGB string, they use BGR instead ,so we need to translate here
    sprintf(buf, "%d %d %d", ptr & 0x0000FF, (ptr & 0x00FF00) >> 8, (ptr & 0xFF0000) >> 16);
    fcitxconf.setValue("UserPhraseColor", QString(buf));
    fcitxconf.setValue("InputColor", QString(buf));
    fcitxconf.setValue("CodeColor", QString(buf));

    val = ssfconf.value("zhongwen_first_color");
    ptr = val.toString().toInt(nullptr, 16);
    sprintf(buf, "%d %d %d", ptr & 0x0000FF, (ptr & 0x00FF00) >> 8, (ptr & 0xFF0000) >> 16);
    fcitxconf.setValue("FirstCandColor", QString(buf));

    val = ssfconf.value("zhongwen_color");
    ptr = val.toString().toInt(nullptr, 16);
    sprintf(buf, "%d %d %d", ptr & 0x0000FF, (ptr & 0x00FF00) >> 8, (ptr & 0xFF0000) >> 16);
    fcitxconf.setValue("OtherColor", QString(buf));
    fcitxconf.setValue("IndexColor", QString(buf));


    fcitxconf.setValue("TipColor", "37 191 237");
    // fcitxconf.setValue("OtherColor", "255 223 231"); // OtherColor = FirstCandidColor = comphint color
    fcitxconf.setValue("ActiveMenuColor", "204 41 76");
    fcitxconf.setValue("InactiveMenuColor", "255 223 231");

    fcitxconf.endGroup();
    ssfconf.endGroup();

    ssfconf.beginGroup("Scheme_H1");
    fcitxconf.beginGroup("SkinInputBar");

    fcitxconf.setValue("BackImg", ssfconf.value("pic"));
    fcitxconf.setValue("FillHorizontal", "Copy");
    fcitxconf.setValue("CursorColor", "255 255 255");
    // TODO: 搞清楚 Margin 的关系，优先搞清楚 SkinInputBar 的
    // TODO: Read the fcitx skin code and draw the margin picture

    // we need to open the picture first
    QString input_bar_file_name = ssfconf.value("pic").toString();
    QImage input_bar_pixmap(dir.filePath(input_bar_file_name));
    if(input_bar_pixmap.isNull()) {
        fprintf(stderr, "FAIL TO OPEN file %s", input_bar_file_name.toStdString().c_str());
        exit(-1);
    }
    int pixw = input_bar_pixmap.width();
    int pixh = input_bar_pixmap.height();
    std::cout << "W=" << pixw << std::endl << "H=" << pixh << std::endl;

    int marge_left = ssfconf.value("layout_horizontal").toStringList().at(1).toInt();
    int marge_right = ssfconf.value("layout_horizontal").toStringList().at(2).toInt();

    // if (marge_left > marge_right) {
    //     int tmp = marge_right;
    //     marge_right = marge_left;
    //     marge_left = tmp;
    // }

    int marge_top = ssfconf.value("layout_vertical").toStringList().at(1).toInt();
    int marge_bot = ssfconf.value("layout_vertical").toStringList().at(2).toInt();

    // if (marge_top > marge_bot) {
    //     int tmp = marge_top;
    //     marge_top = marge_bot;
    //     marge_bot = tmp;
    // }

    // Try this version

    // TODO: Fix input pos and output pos
    int pinyin_marge_top = ssfconf.value("pinyin_marge").toStringList().at(0).toInt();
    int pinyin_marge_left = ssfconf.value("pinyin_marge").toStringList().at(2).toInt();
    int pinyin_marge_right = ssfconf.value("pinyin_marge").toStringList().at(3).toInt();

    int zhongwen_marge_bot = ssfconf.value("zhongwen_marge").toStringList().at(1).toInt();

    int sep_zhongwen = ssfconf.value("zhongwen_marge").toStringList().at(0).toInt();
    int sep_pinyin = ssfconf.value("pinyin_marge").toStringList().at(1).toInt();

    // TODO: 在搜狗里对于输入区域的左右对齐是有另外调整的，可是在 fcitx 里就只有管理九宫格的 MargineLeft 和 Right 同时用来控制对齐，这其实就有些难受了
    fcitxconf.setValue("MarginLeft", marge_left);
    fcitxconf.setValue("MarginRight",  marge_right);

    fcitxconf.setValue("MarginTop", pinyin_marge_top + font_pix);
    fcitxconf.setValue("MarginBottom", pixh - font_pix - sep_zhongwen - font_pix - pinyin_marge_top);

    //  val < 0 means offset up, > 0 means down
    // fcitxconf.setValue("InputPos", pinyin_marge_top - marge_top);
    // fcitxconf.setValue("OutputPos", pixh - marge_bot - zhongwen_marge_bot);

    fcitxconf.setValue("InputPos", -sep_pinyin);
    // fcitxconf.setValue("OutputPos", pixh - pinyin_marge_top - zhongwen_marge_bot - font_pix);
    fcitxconf.setValue("OutputPos", font_pix + sep_zhongwen);

    // TODO: generate back arrow and forward arrow for fcitx
    // TODO: STUB NOW
    fcitxconf.setValue("BackArrow", "prev.png");
    fcitxconf.setValue("ForwardArrow", "next.png");
    fcitxconf.setValue("BackArrowX", 140);
    fcitxconf.setValue("BackArrowY", 175);
    fcitxconf.setValue("ForwardArrowX", 130);
    fcitxconf.setValue("ForwardArrowY", 175);

    fcitxconf.endGroup();
    ssfconf.endGroup();

    ssfconf.beginGroup("StatusBar");
    fcitxconf.beginGroup("SkinMainBar");

    fcitxconf.setValue("BackImg", ssfconf.value("pic"));
    QStringList cn_en_list = ssfconf.value("cn_en").toStringList();
    // QSettings IS SHIT! The below line won't output anything
    std::cout << ssfconf.value("cn_en").toString().split(",").join(",").toStdString() << std::endl;
    fcitxconf.setValue("Active", cn_en_list.at(0));
    fcitxconf.setValue("Eng", cn_en_list.at(1));
    fcitxconf.setValue("Logo", "logo.png");

    // TODO: fix the margin
    fcitxconf.setValue("MarginLeft", 45);
    fcitxconf.setValue("MarginRight", 50);
    fcitxconf.setValue("MarginBottom", 25);
    fcitxconf.setValue("MarginTop", 40);

    fcitxconf.endGroup();
    ssfconf.endGroup();

    fcitxconf.beginGroup("SkinKeyboard");
    fcitxconf.setValue("KeyColor", "105 21 20");
    fcitxconf.endGroup();

    fcitxconf.beginGroup("SkinTrayIcon");
    fcitxconf.setValue("Active", "active.png");
    fcitxconf.setValue("Inactive", "inactive.png");
    fcitxconf.endGroup();

    // TODO: fix the margin, not so high priority
    fcitxconf.beginGroup("SkinMenu");
    fcitxconf.setValue("BackImg", "menu.png");
    fcitxconf.setValue("MarginLeft", 50);
    fcitxconf.setValue("MarginRight", 180);
    fcitxconf.setValue("MarginBottom", 162);
    fcitxconf.setValue("MarginTop", 27);
    fcitxconf.setValue("ActiveColor", "255 223 231");
    fcitxconf.setValue("LineColor", "255 223 231");
    fcitxconf.setValue("FillHorizontal", "Copy");
    fcitxconf.endGroup();

    return 0;
}

