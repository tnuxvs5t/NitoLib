#!/usr/bin/env python3
"""Build review artifacts from the verified sources. Requires reportlab for PDF."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

from reportlab.lib.colors import HexColor
from reportlab.lib.pagesizes import A4
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--validation', type=Path, required=True)
args = parser.parse_args()
out = args.out.resolve()
out.mkdir(parents=True, exist_ok=True)
validation = json.loads(args.validation.read_text())
assert validation['status'] == 'passed'
assert {row['mode'] for row in validation['runs']} == {'debug', 'opt', 'san'}
for relative, expected in validation['source_sha256'].items():
    assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == expected, relative

modules = {p.name: p.read_text() for p in sorted((ROOT / 'include').glob('*.hpp'))}
examples = {p.stem: p.read_text() for p in sorted((ROOT / 'examples').glob('*.cpp'))}
payload = json.dumps({'modules': modules, 'examples': examples, 'validation': validation}, ensure_ascii=False)
payload = payload.replace('<', '\\u003c').replace('>', '\\u003e').replace('&', '\\u0026')
site = out / 'Nitori-design-showcase'
site.mkdir(exist_ok=True)
template = (ROOT / 'showcase.html').read_text()
assert template.count('__SHOWCASE_DATA__') == 1
(site / 'index.html').write_text(template.replace('__SHOWCASE_DATA__', payload))
(out / 'Nitori-design-plan.md').write_text((ROOT / 'DESIGN.md').read_text())
if args.validation.resolve() != (out / 'Nitori-design-validation.json').resolve():
    shutil.copyfile(args.validation, out / 'Nitori-design-validation.json')

pdfmetrics.registerFont(TTFont('CN', '/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf'))
pdfmetrics.registerFont(TTFont('Mono', '/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'))
pdfmetrics.registerFont(TTFont('Sans', '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'))
pdfmetrics.registerFont(TTFont('SansBold', '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'))
width, height = A4
margin = 38
ink, green, muted = HexColor('#172c2a'), HexColor('#12644c'), HexColor('#59655e')
pdf_path = out / 'Nitori-design-notebook.pdf'
pdf = canvas.Canvas(str(pdf_path), pagesize=A4)
pdf.setTitle('Nitori Design Study 01 - Contest Notebook Specimen')
pdf.setAuthor('Nitori design lab')
pages = []

def paragraph(text, y, size=10, leading=16):
    pdf.setFillColor(ink)
    def font(c): return 'Sans' if ord(c) < 128 else 'CN'
    def line_width(value):
        return sum(pdfmetrics.stringWidth(c, font(c), size) for c in value)
    def draw(value, baseline):
        x = margin
        for c in value:
            face = font(c)
            pdf.setFont(face, size)
            pdf.drawString(x, baseline, c)
            x += pdfmetrics.stringWidth(c, face, size)
    line = ''
    for c in text:
        if c == '\n' or line_width(line + c) > width - 2 * margin:
            draw(line, y)
            y -= leading
            line = '' if c == '\n' else c
        else: line += c
    if line:
        draw(line, y)
        y -= leading
    return y

def start(title, subtitle):
    pdf.setFillColor(green)
    pdf.setFont('SansBold', 10)
    pdf.drawString(margin, height - 36, 'NITORI / DESIGN STUDY 01')
    pdf.setFillColor(ink)
    pdf.setFont('SansBold', 19)
    pdf.drawString(margin, height - 66, title)
    pdf.setStrokeColor(HexColor('#c4cfbe'))
    pdf.line(margin, height - 80, width - margin, height - 80)
    return paragraph(subtitle, height - 100, 9, 14) - 8

def finish(label):
    pdf.setFillColor(muted)
    pdf.setFont('Sans', 8)
    pdf.drawString(margin, 25, 'C++23 | int positions | [left,right) | experimental scope')
    pdf.drawRightString(width - margin, 25, str(len(pages) + 1))
    pages.append(label)
    pdf.showPage()

y = start('Readable under pressure.', 'CF 与 ICPC 纸面设计样张 · 2026-09-13')
for text in [
    '这是五个独立实验模块与四个完整配方的审阅样张，不是完整 ICPC notebook，也没有替换正式 V3。',
    '纸面目标：看见状态，找到不变量，知道在题目中改哪一处。模块代码与已验证源码逐行一致；跨页时按行号连续抄录。',
    '基础约定：C++23；编号 int；值和坐标另选宽度；区间半开。数学定律、存储寿命与合法输入均是调用者契约。',
    '编译模块：每个 include/ 头文件独立成立，只依赖标准库。配方的相对 include 指向同目录树中的模块。纸面抄录时可把所需完整模块放在配方前，并去掉已粘贴模块的相对 include。',
    '验证证据：五个模块独立编译；四个配方输出核对；五个性质/深链程序各经过 debug、opt、ASan+UBSan；30 万点链固定 8 MiB 栈。',
    '范围边界：HLD 只接受连通无向树；lazy 尚无边界搜索/Beats/位置相关动作；span 不延长 owner 寿命；最短路的有限距离严格小于 INF。',
    '未完成的验收：人工从打印页重新录入、具体赛事编译环境与页数裁剪。自动测试不能替代这些检查。',
]:
    y = paragraph(text, y, 10, 17) - 14
finish('cover')

contracts = {
    'sequence.hpp': '排序计划与分段边界显式持有。gather 借用两个 span；存储重分配和销毁使其失效。只有 argsort/run_bounds 分配结果数组。',
    'graph.hpp': '稠密顶点 [0,n)。next/to/cost 为调用期端口。Dijkstra 权值非负，有限距离严格小于 INF；结果默认保存前驱。',
    'hld.hpp': '连通无向树，next 可重复。元数据全部自有；显式堆栈处理深链。path 保序；边模式按深点存储并排除 LCA。',
    'segtree.hpp': 'Monoid：value_type、id、join，结合且保序。边界 predicate 在单位元为真，沿扩展最多由真变假一次。',
    'lazy_segtree.hpp': 'Algebra：value_type/tag_type、id、join、apply、compose(newer,older)。普通同构动作，分配律成立；不承担 Beats。',
}
for name, source in modules.items():
    lines = source.splitlines()
    at, part = 0, 1
    while at < len(lines):
        y = start(name + ('' if part == 1 else f' / continued {part}'), contracts[name])
        while at < len(lines) and y > 49:
            label = f'{at + 1:3} '
            line = lines[at]
            size = 8.0
            available = width - 2 * margin - 24
            measured = pdfmetrics.stringWidth(line, 'Mono', size)
            if measured > available: size *= available / measured
            assert size >= 7.0, f'Code line too long for readable paper: {name}:{at+1}'
            pdf.setFont('Mono', 7)
            pdf.setFillColor(muted)
            pdf.drawString(margin, y, label)
            pdf.setFont('Mono', size)
            pdf.setFillColor(ink)
            pdf.drawString(margin + 24, y, line)
            y -= 10.4
            at += 1
        finish(name)
        part += 1

instructions = {
    'grouping': '改 Record 与 key/equal。计划不改原数组，等键保留原位置顺序。要保存结果时复制数据；groups 是边界快照。',
    'shortest_path': '改 Edge、邻接与 INF。距离类型与路径和须适合题目范围；前驱只表示顶点路径，不区分平行边身份。',
    'ordered_path': '改 Both 与 Concat。forward/backward 必须由相反顺序合并。这里的字符串仅作顺序见证；真实复杂度需计字符串长度。',
    'affine_sum': '改 value_type 与算术。compose(newer,older) 先旧后新。long long 演示需保证不溢出；模数题替换数值类型。',
}
for name, source in examples.items():
    y = start(name + '.cpp', instructions[name])
    for line in source.splitlines():
        if y < 52: finish(name); y = start(name + ' / continued', instructions[name])
        size = min(8.0, (width - 2 * margin) / max(1, pdfmetrics.stringWidth(line, 'Mono', 1)))
        assert size >= 7.0
        pdf.setFillColor(ink); pdf.setFont('Mono', size); pdf.drawString(margin, y, line); y -= 11
    y -= 20
    y = paragraph('已验证输出', y, 10, 17)
    for line in validation['examples'][name].splitlines():
        pdf.setFillColor(green); pdf.setFont('Mono', 10); pdf.drawString(margin, y, line); y -= 15
    finish(name)
pdf.save()

metrics = {name: {'lines': len(text.splitlines()), 'bytes': len(text.encode()), 'local_dependencies': []}
           for name, text in modules.items()}
(out / 'Nitori-design-artifacts.json').write_text(json.dumps({
    'paper_pages': len(pages), 'page_contents': pages, 'modules': metrics,
    'module_total_lines': sum(v['lines'] for v in metrics.values()),
    'coverage_note': 'Five experimental modules only; not full V3 feature parity.',
}, indent=2, ensure_ascii=False) + '\n')

bundle = out / 'Nitori-design-showcase.zip'
with zipfile.ZipFile(bundle, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
    for directory in (ROOT, ROOT.parent / 'src-v3'):
        for path in sorted(directory.rglob('*')):
            if path.is_file() and '__pycache__' not in path.parts:
                archive.write(path, Path('NitoriSTL') / path.relative_to(ROOT.parent))
    for path in [site / 'index.html', pdf_path, out / 'Nitori-design-plan.md',
                 out / 'Nitori-design-validation.json', out / 'Nitori-design-artifacts.json',
                 out / 'NitoriSTL-before-cleansing.manifest.json']:
        if path.exists(): archive.write(path, Path('outputs') / path.relative_to(out))
    archive.writestr('README.txt',
        'Nitori design study 01\n\nRun: cd NitoriSTL && python3 design-lab/run.py\n'
        'src-v3 is an unchanged baseline for cost comparison only. Lab modules do not include it.\n'
        'Open outputs/Nitori-design-showcase/index.html; PDF and design notes are beside it.\n'
        'The full pre-change repository archive is a separate deliverable, not duplicated in this bundle.\n'
        'The archive download and bundle self-download links require the full sibling outputs directory.\n')
print(json.dumps({'site': str(site / 'index.html'), 'pdf': str(pdf_path), 'pages': len(pages),
                  'bundle': str(bundle), 'module_lines': sum(len(s.splitlines()) for s in modules.values())}, indent=2))
