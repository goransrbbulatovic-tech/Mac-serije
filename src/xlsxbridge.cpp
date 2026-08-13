#include "xlsxbridge.h"
#include <QProcess>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>

static const char* IMPORT_SCRIPT = R"PYEOF(
# -*- coding: utf-8 -*-
import sys, json, pandas as pd, os, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

def parse_size(s):
    if not s or s == 'nan': return ''
    return str(s).strip()

def parse_year(s):
    if not s or s == 'nan': return ''
    s = str(s).strip().replace('(','').replace(')','').strip()
    return s if s != 'nan' else ''

try:
    xl = pd.read_excel(sys.argv[1], sheet_name=None, dtype=str)
    result = []
    for sheet_name, df in xl.items():
        cat = 'DOMACE' if any(x in sheet_name.upper() for x in ['DOMA','DOM']) else 'STRANE'
        df.columns = [str(c).strip().upper() for c in df.columns]
        col_map = {}
        for c in df.columns:
            cu = c.upper()
            if 'SERIJ' in cu or 'NAZIV' in cu or 'IME' in cu or 'NAME' in cu: col_map['name'] = c
            elif 'VELI' in cu or 'SIZE' in cu: col_map['size'] = c
            elif 'GOD' in cu or 'YEAR' in cu: col_map['year'] = c
            elif 'HARD' in cu or 'HDD' in cu or 'DISK' in cu: col_map['hdd'] = c
        for _, row in df.iterrows():
            name = str(row.get(col_map.get('name','SERIJA'), '')).strip()
            if not name or name == 'nan' or name == 'SERIJA': continue
            size = parse_size(row.get(col_map.get('size','VELICINA'), ''))
            year = parse_year(row.get(col_map.get('year','GODINA'), ''))
            hdd  = str(row.get(col_map.get('hdd','HARD DISK'), '')).strip()
            if hdd == 'nan': hdd = ''
            result.append({'name':name,'size':size,'year':year,'hard_disk':hdd,'category':cat})
    print(json.dumps(result, ensure_ascii=False))
except Exception as e:
    print(json.dumps({'error': str(e)}, ensure_ascii=False), file=sys.stderr)
    sys.exit(1)
)PYEOF";

static const char* EXPORT_SCRIPT = R"PYEOF(
# -*- coding: utf-8 -*-
import sys, json, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')
from openpyxl import Workbook
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

with open(sys.argv[1],'r',encoding='utf-8') as f: rows=json.load(f)
wb=Workbook(); wb.remove(wb.active)
groups={'DOMACE':[],'STRANE':[]}
for r in rows: groups[r.get('category','STRANE')].append(r)
sheet_names={'DOMACE':'DOMACE SERIJE','STRANE':'STRANE SERIJE'}
HDR_BG='1A0033'; HDR_FG='FFFFFF'; ALT1='0D0D1A'; ALT2='14142A'; TEXT_COL='E0E0F0'; GOLD='F5C518'
thin=Side(style='thin',color='2A2A45')
border=Border(left=thin,right=thin,top=thin,bottom=thin)
cols=['#','NAZIV SERIJE','KATEGORIJA','GODINA','VELICINA','HARD DISK','IMDB OCJENA','ZANR','GLEDANO']
widths=[5,42,12,14,12,18,12,22,10]
for cat,items in groups.items():
    ws=wb.create_sheet(sheet_names[cat])
    for col_i,(col_name,w) in enumerate(zip(cols,widths),1):
        cell=ws.cell(row=1,column=col_i,value=col_name)
        cell.font=Font(name='Segoe UI',bold=True,color=HDR_FG,size=11)
        cell.fill=PatternFill('solid',fgColor=HDR_BG)
        cell.alignment=Alignment(horizontal='center',vertical='center',wrap_text=True)
        cell.border=border
        ws.column_dimensions[get_column_letter(col_i)].width=w
    ws.row_dimensions[1].height=30; ws.freeze_panes='A2'
    for row_i,item in enumerate(items,2):
        bg=ALT1 if row_i%2==0 else ALT2
        vals=[row_i-1,item.get('name',''),'Domaca' if item.get('category')=='DOMACE' else 'Strana',
              item.get('year',''),item.get('size',''),item.get('hard_disk',''),
              item.get('imdb_rating',0) if item.get('imdb_rating',0)>0 else '-',
              item.get('genre',''),'DA' if item.get('watched') else '']
        for col_i,val in enumerate(vals,1):
            cell=ws.cell(row=row_i,column=col_i,value=val)
            cell.fill=PatternFill('solid',fgColor=bg); cell.border=border
            align_h='center' if col_i in [1,3,4,5,7,9] else 'left'
            cell.alignment=Alignment(horizontal=align_h,vertical='center')
            if col_i==7 and isinstance(val,float):
                color=GOLD if val>=7 else ('FFA502' if val>=5 else 'FF4757')
                cell.font=Font(name='Segoe UI',color=color,bold=True,size=11)
            else: cell.font=Font(name='Segoe UI',color=TEXT_COL,size=10)
        ws.row_dimensions[row_i].height=20
    last=len(items)+2
    ws.cell(row=last,column=1,value='UKUPNO:').font=Font(bold=True,color=GOLD,size=11)
    ws.cell(row=last,column=2,value=len(items)).font=Font(bold=True,color=GOLD,size=11)
wb.save(sys.argv[2]); print('OK')
)PYEOF";

XlsxBridge::XlsxBridge(QObject *parent) : QObject(parent) {}

QString XlsxBridge::pythonExecutable() {
    for(const QString &py:{"python3","python"}){
        QProcess p; p.start(py,{"--version"});
        if(p.waitForFinished(3000)&&p.exitCode()==0) return py;
    }
    return "python3";
}

bool XlsxBridge::runPython(const QString &script, const QStringList &args,
                             QString &output, QString &errOut) {
    QTemporaryFile tf;
    tf.setFileTemplate(QDir::tempPath()+"/ai_XXXXXX.py");
    if(!tf.open()){errOut="Cannot create temp file";return false;}
    tf.write(script.toUtf8()); tf.flush();
    QString scriptPath=tf.fileName(); tf.close();

    QProcess proc;
    QProcessEnvironment env=QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING","utf-8");
    env.insert("PYTHONUTF8","1");
    proc.setProcessEnvironment(env);

    QStringList fullArgs={scriptPath}; fullArgs+=args;
    proc.start(pythonExecutable(),fullArgs);
    if(!proc.waitForStarted(5000)){errOut="Cannot start Python";return false;}
    if(!proc.waitForFinished(60000)){errOut="Python timed out";return false;}
    output=QString::fromUtf8(proc.readAllStandardOutput());
    errOut=QString::fromUtf8(proc.readAllStandardError());
    if(proc.exitCode()!=0) return false;
    return true;
}

QList<SeriesData> XlsxBridge::importFromXlsx(const QString &filePath, QString &error) {
    QList<SeriesData> list; QString output, errOut;
    if(!runPython(IMPORT_SCRIPT,{filePath},output,errOut)){
        error=errOut.isEmpty()?"Python script failed":errOut; return list;}
    QJsonParseError pe;
    auto doc=QJsonDocument::fromJson(output.toUtf8(),&pe);
    if(pe.error!=QJsonParseError::NoError){error="JSON parse error: "+pe.errorString();return list;}
    if(!doc.isArray()){
        if(doc.isObject()&&doc.object().contains("error")) error=doc.object()["error"].toString();
        return list;}
    for(const QJsonValue &v:doc.array()){
        QJsonObject o=v.toObject(); SeriesData s;
        s.name=o["name"].toString().trimmed(); s.size=o["size"].toString().trimmed();
        s.year=o["year"].toString().trimmed(); s.hardDisk=o["hard_disk"].toString().trimmed();
        s.category=o["category"].toString();
        if(s.name.isEmpty()) continue; list.append(s);
    }
    return list;
}

bool XlsxBridge::exportToXlsx(const QString &filePath, const QList<SeriesData> &data, QString &error) {
    QJsonArray arr;
    for(const SeriesData &s:data){
        QJsonObject o; o["name"]=s.name; o["size"]=s.size; o["year"]=s.year;
        o["hard_disk"]=s.hardDisk; o["category"]=s.category;
        o["imdb_rating"]=s.imdbRating; o["genre"]=s.genre; o["watched"]=s.watched;
        arr.append(o);
    }
    QTemporaryFile df; df.setFileTemplate(QDir::tempPath()+"/ai_data_XXXXXX.json");
    if(!df.open()){error="Cannot create temp data file";return false;}
    df.write(QJsonDocument(arr).toJson()); df.flush();
    QString dataPath=df.fileName(); df.close();
    QString output, errOut;
    if(!runPython(EXPORT_SCRIPT,{dataPath,filePath},output,errOut)){
        error=errOut.isEmpty()?"Export failed":errOut; return false;}
    return true;
}
