#include "decodebitstreamcommand.h"
#include "io/analyzermsgsender.h"
#include "model/modellocator.h"
#include "parsers/bitstreamparser.h"
#include "parsers/spsparser.h"
#include "parsers/decodergeneralparser.h"
#include "parsers/cupuparser.h"
#include "parsers/predparser.h"
#include "parsers/mvparser.h"
#include "parsers/mergeparser.h"
#include "parsers/intraparser.h"
#include "events/eventnames.h"
#include "exceptions/decodingfailexception.h"

DecodeBitstreamCommand::DecodeBitstreamCommand(QObject *parent) :
    AbstractCommand(parent)
{
}

bool DecodeBitstreamCommand::execute( CommandRequest& rcRequest, CommandRespond& rcRespond )
{
    ModelLocator* pModel = ModelLocator::getInstance();

    /// *****STEP 0 : Request*****
    QVariant vValue;
    rcRequest.getParameter("filename", vValue);
    QString strFilename = vValue.toString();
    rcRequest.getParameter("sequence", vValue);
    QString strSequenceSelector = vValue.toString();
    rcRequest.getParameter("version", vValue);
    int iVersion = vValue.toInt();
    rcRequest.getParameter("skip_decode", vValue);
    bool bSkipDecode = vValue.toBool();
    rcRequest.getParameter("decoder_folder", vValue);
    QString strDecoderPath = vValue.toString();
    rcRequest.getParameter("output_folder", vValue);
    QString strDecoderOutputPath = vValue.toString();
    int iSequenceIndex = pModel->getSequenceManager().getAllSequences().size();
    strDecoderOutputPath += QString("/%1").arg(iSequenceIndex);




    /// Init Sequence

//    ComSequence* pcSequence = NULL;
//    if(strSequenceSelector == "testing_sequence")
//    {
//        pModel->getSequenceManager().setSequenceSelector(SEQUENCE_0);
//    }
//    else if(strSequenceSelector == "benchmark_sequence")
//    {
//        pModel->getSequenceManager().setSequenceSelector(SEQUENCE_1);
//    }
//    else
//    {
//        AnalyzerMsgSender::getInstance()->msgOut("Undefined sequence selector", GITL_MSG_ERROR);
//        return false;
//    }

    //TODO BUG Memory Leaking when exception happens
    ComSequence* pcSequence = new ComSequence();
    pcSequence->init();

    /// *****STEP 1 : Use the special decoder to parse bitstream*****
    /// call decoder process to decode bitstream to YUV and output text info
    GitlEvent cDecodingStageInfo(g_strCmdInfoEvent);
    bool bSuccess = false;
    if( !bSkipDecode )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(1/9)Start Decoding Bitstream...");
        dispatchEvt(&cDecodingStageInfo);
        BitstreamParser cBitstreamParser;
        bSuccess = cBitstreamParser.parseFile(strDecoderPath,
                                              iVersion,
                                              strFilename,
                                              strDecoderOutputPath,
                                              pcSequence);
        if( !bSuccess )
            throw DecodingFailException();
        AnalyzerMsgSender::getInstance()->msgOut("decoding finished", GITL_MSG_DEBUG);
    }
    else
    {
        bSuccess = true;
        AnalyzerMsgSender::getInstance()->msgOut("decoding skiped", GITL_MSG_DEBUG);
    }


    /// *****STEP 2 : Parse the txt file generated by decoder*****
    /// Parse decoder_sps.txt
    QString strSPSFilename = strDecoderOutputPath + "/decoder_sps.txt";
    if( bSuccess )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(2/9)Start Parsing Sequence Parameter Set...");
        dispatchEvt(&cDecodingStageInfo);
        QFile cSPSFile(strSPSFilename);
        cSPSFile.open(QIODevice::ReadOnly);
        QTextStream cSPSTextStream(&cSPSFile);
        SpsParser cSpsParser;
        bSuccess = cSpsParser.parseFile( &cSPSTextStream, pcSequence );
        cSPSFile.close();
        AnalyzerMsgSender::getInstance()->msgOut("SPS file parsing finished", GITL_MSG_DEBUG);
    }
    /// Parse decoder_general.txt
    QString strGeneralFilename = strDecoderOutputPath + "/decoder_general.txt";
    if( bSuccess )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(3/9)Start Parsing Decoder Std Output File...");
        dispatchEvt(&cDecodingStageInfo);
        QFile cGeneralFile(strGeneralFilename);
        cGeneralFile.open(QIODevice::ReadOnly);
        QTextStream cGeneralTextStream(&cGeneralFile);
        DecoderGeneralParser cDecoderGeneralParser;
        bSuccess = cDecoderGeneralParser.parseFile( &cGeneralTextStream, pcSequence );
        cGeneralFile.close();
        AnalyzerMsgSender::getInstance()->msgOut("Decoder general file parsing finished", GITL_MSG_DEBUG);
    }

    /// Parse decoder_cupu.txt
    QString strCUPUFilename = strDecoderOutputPath + "/decoder_cupu.txt";
    if( bSuccess )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(4/9)Start Parsing CU & PU Structure...");
        dispatchEvt(&cDecodingStageInfo);
        QFile cCUPUFile(strCUPUFilename);
        cCUPUFile.open(QIODevice::ReadOnly);
        QTextStream cCUPUTextStream(&cCUPUFile);
        CUPUParser cCUPUParser;
        bSuccess = cCUPUParser.parseFile( &cCUPUTextStream, pcSequence );
        cCUPUFile.close();
        AnalyzerMsgSender::getInstance()->msgOut("CU&PU file parsing finished", GITL_MSG_DEBUG);
    }

    /// Parse decoder_pred.txt
    QString strPredFilename = strDecoderOutputPath + "/decoder_pred.txt";
    if( bSuccess )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(5/9)Start Parsing Predction Mode...");
        dispatchEvt(&cDecodingStageInfo);
        QFile cPredFile(strPredFilename);
        cPredFile.open(QIODevice::ReadOnly);
        QTextStream cPredTextStream(&cPredFile);
        PredParser cPredParser;
        bSuccess = cPredParser.parseFile( &cPredTextStream, pcSequence );
        cPredFile.close();
        AnalyzerMsgSender::getInstance()->msgOut("Prediction file parsing finished", GITL_MSG_DEBUG);
    }

    /// Parse decoder_mv.txt
    QString strMVFilename = strDecoderOutputPath + "/decoder_mv.txt";
    if( bSuccess )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(6/9)Start Parsing Motion Vectors...");
        dispatchEvt(&cDecodingStageInfo);
        QFile cMVFile(strMVFilename);
        cMVFile.open(QIODevice::ReadOnly);
        QTextStream cMVTextStream(&cMVFile);
        MVParser cMVParser;
        bSuccess = cMVParser.parseFile( &cMVTextStream, pcSequence );
        cMVFile.close();
        AnalyzerMsgSender::getInstance()->msgOut("MV file parsing finished", GITL_MSG_DEBUG);
    }

    /// Parse decoder_merge.txt
    QString strMergeFilename = strDecoderOutputPath + "/decoder_merge.txt";
    if( bSuccess )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(7/9)Start Parsing Motion Merge Info...");
        dispatchEvt(&cDecodingStageInfo);
        QFile cMergeFile(strMergeFilename);
        cMergeFile.open(QIODevice::ReadOnly);
        QTextStream cMergeTextStream(&cMergeFile);
        MergeParser cMergeParser;
        bSuccess = cMergeParser.parseFile( &cMergeTextStream, pcSequence );
        cMergeFile.close();
        AnalyzerMsgSender::getInstance()->msgOut("Merge file parsing finished", GITL_MSG_DEBUG);
    }

    /// Parse decoder_intra.txt
    QString strIntraFilename = strDecoderOutputPath + "/decoder_intra.txt";
    if( bSuccess )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(8/9)Start Parsing Intra Info...");
        dispatchEvt(&cDecodingStageInfo);
        QFile cIntraFile(strIntraFilename);
        cIntraFile.open(QIODevice::ReadOnly);
        QTextStream cIntraTextStream(&cIntraFile);
        IntraParser cIntraParser;
        bSuccess = cIntraParser.parseFile( &cIntraTextStream, pcSequence );
        cIntraFile.close();
        AnalyzerMsgSender::getInstance()->msgOut("Intra file parsing finished", GITL_MSG_DEBUG);
    }

    ///*****STEP 3 : Open decoded YUV sequence*****
    pModel->getSequenceManager().addSequence(pcSequence);
    pModel->getSequenceManager().setCurrentSequence(pcSequence);

    QString strYUVFilename = strDecoderOutputPath + "/decoder_yuv.yuv";
    int iWidth  = pModel->getSequenceManager().getCurrentSequence().getWidth();
    int iHeight = pModel->getSequenceManager().getCurrentSequence().getHeight();
    QPixmap* pcFramePixmap = NULL;
    if( bSuccess )
    {
        cDecodingStageInfo.getEvtData().setParameter("message", "(9/9)Reding & Drawing Reconstructed YUV...");
        dispatchEvt(&cDecodingStageInfo);
        pModel->getFrameBuffer().setYUVFile(strYUVFilename, iWidth, iHeight);
        pcFramePixmap = pModel->getFrameBuffer().getFrame(0);   ///< Read Frame Buffer
        pcFramePixmap = pModel->getDrawEngine().drawFrame(pcSequence, 0, pcFramePixmap);  ///< Draw Frame Buffer

    }


    ///*****STEP 4 : Respond*****
    int iCurrentPoc = pModel->getFrameBuffer().getPoc();
    int iTotalFrame = pModel->getSequenceManager().getCurrentSequence().getTotalFrames();
    rcRespond.setParameter("picture",    QVariant::fromValue((void*)(pcFramePixmap)) );
    rcRespond.setParameter("current_frame_poc", iCurrentPoc);
    rcRespond.setParameter("total_frame_num", iTotalFrame);


    /// nofity sequence list to update
    GitlEvent cSequenceChangedEvt(g_strSquencesListChanged);
    QVector<ComSequence*>* ppcSequences = &(pModel->getSequenceManager().getAllSequences());
    cSequenceChangedEvt.getEvtData().setParameter("sequences",QVariant::fromValue((void*)ppcSequences));
    cSequenceChangedEvt.getEvtData().setParameter("current_sequence",QVariant::fromValue((void*)pcSequence));

    dispatchEvt(&cSequenceChangedEvt);



    return bSuccess;
}
