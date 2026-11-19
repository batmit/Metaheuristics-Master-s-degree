"""Gera o guia PDF a partir de docs/algoritmo.md; requer somente ReportLab."""
from pathlib import Path
import re
from xml.sax.saxutils import escape

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.platypus import SimpleDocTemplate, Paragraph, PageBreak, Preformatted

ROOT = Path(__file__).resolve().parents[1]
NAVY = colors.HexColor('#15324A')
TEAL = colors.HexColor('#147D85')


def inline(text):
    """Converte apenas enfase e codigo do Markdown, escapando caracteres XML."""
    text = escape(text)
    text = re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', text)
    return re.sub(r'`([^`]+)`', r'<font name="Courier">\1</font>', text)


def moldura(canvas, document):
    """Desenha cabecalho e rodape fora da area de texto, com numero da pagina."""
    width, height = A4
    canvas.saveState()
    canvas.setFillColor(NAVY)
    canvas.rect(0, height-35, width, 35, fill=1, stroke=0)
    canvas.setFillColor(colors.white)
    canvas.setFont('Helvetica-Bold', 9)
    canvas.drawString(44, height-22, 'PDSVRP-C  /  SIMULATED ANNEALING')
    canvas.setStrokeColor(TEAL)
    canvas.line(44, 39, width-44, 39)
    canvas.setFillColor(NAVY)
    canvas.setFont('Helvetica', 8)
    canvas.drawString(44, 25, 'Guia da implementacao | 06/11/2026')
    canvas.drawRightString(width-44, 25, str(document.page))
    canvas.restoreState()


def main():
    """Cada titulo principal inicia uma pagina; paragrafos fluem sem coordenadas fixas."""
    target = ROOT/'output/pdf/algoritmo_pdsvrp_c.pdf'
    target.parent.mkdir(parents=True, exist_ok=True)
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle(name='TitleGuide', fontName='Helvetica-Bold', fontSize=21,
                              leading=25, textColor=NAVY, spaceAfter=18))
    styles.add(ParagraphStyle(name='SectionGuide', fontName='Helvetica-Bold', fontSize=12,
                              leading=16, textColor=TEAL, spaceBefore=10, spaceAfter=7))
    styles.add(ParagraphStyle(name='BodyGuide', fontName='Helvetica', fontSize=10,
                              leading=14.2, textColor=NAVY, alignment=TA_LEFT, spaceAfter=9))
    styles.add(ParagraphStyle(name='BulletGuide', parent=styles['BodyGuide'],
                              leftIndent=12, firstLineIndent=-8, spaceAfter=5))
    styles.add(ParagraphStyle(name='CodeGuide', fontName='Courier', fontSize=8.5,
                              leading=12, textColor=NAVY, backColor=colors.HexColor('#EEF5F6'),
                              borderPadding=10, spaceBefore=6, spaceAfter=12))
    story, paragraph, code = [], [], []
    in_code = False

    def flush():
        """Une linhas do mesmo paragrafo para que o ReportLab quebre o texto."""
        if paragraph:
            story.append(Paragraph(inline(' '.join(paragraph)), styles['BodyGuide']))
            paragraph.clear()

    for line in (ROOT/'docs/algoritmo.md').read_text(encoding='utf-8').splitlines():
        if line.startswith('```'):
            flush()
            if in_code:
                story.append(Preformatted('\n'.join(code), styles['CodeGuide']))
                code.clear()
            in_code = not in_code
        elif in_code:
            code.append(line)
        elif line.startswith('# '):
            flush()
            if story: story.append(PageBreak())
            story.append(Paragraph(inline(line[2:]), styles['TitleGuide']))
        elif line.startswith('## '):
            flush()
            story.append(Paragraph(inline(line[3:]), styles['SectionGuide']))
        elif line.startswith('- ') or re.match(r'^\d+\. ', line):
            flush()
            text = '- '+line[2:] if line.startswith('- ') else line
            story.append(Paragraph(inline(text), styles['BulletGuide']))
        elif not line.strip():
            flush()
        else:
            paragraph.append(line)
    flush()
    document = SimpleDocTemplate(str(target), pagesize=A4, leftMargin=44, rightMargin=44,
                                 topMargin=58, bottomMargin=53,
                                 title='PDSVRP-C: guia simplificado do algoritmo',
                                 author='Projeto PDSVRP-C')
    document.build(story, onFirstPage=moldura, onLaterPages=moldura)
    print(target)


if __name__ == '__main__':
    main()
