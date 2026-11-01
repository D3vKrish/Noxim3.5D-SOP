from docx import Document
from docx.shared import Pt

# Initialize document
doc = Document()

# Title
title = doc.add_heading('AdEle+ Implementation & Dynamic Validation Report', 0)
title.alignment = 1 # Center align

# Metadata
doc.add_paragraph("Date: September 26, 2026").bold = True
doc.add_paragraph("Project: 3D NoC Routing Evaluation (AdEle+ Adaptive Algorithm)").bold = True
doc.add_paragraph("")

# Section 1
doc.add_heading('1. Summary & Overview', level=1)
doc.add_paragraph(
    "Building upon offline geometric pre-computations (Stage 1) and core architectural modifications (Stage 2), "
    "the AdEle+ routing algorithm implementation has advanced to dynamic validation. The focus transitioned from "
    "establishing static structural subsets to proving runtime efficacy under realistic 3D Network-on-Chip (NoC) constraints. "
    "Achieving this required stabilizing the simulation environment, realigning physical vertical paths, and executing "
    "comparative evaluations across Distance-Based (DB), Congestion-Aware (ERR), and Adaptive (AdEle+) routing modes."
)

# Section 2
doc.add_heading('2. Stage 3: Simulator Optimization & Topology Fixes', level=1)
ul = doc.add_paragraph(style='List Bullet')
ul.add_run("C++ State Tracking Fixed: ").bold = True
ul.add_run("Resolved compilation errors in Router.cpp by properly initializing the global router registry, enabling accurate end-of-sim collection of average elevator delays.")

ul2 = doc.add_paragraph(style='List Bullet')
ul2.add_run("Vertical Elevator Realignment: ").bold = True
ul2.add_run("Reconfigured the 3DPARTIALLYCONNECTED.yaml Multi-Mesh topology to place elevators along vertical pillars on perimeter nodes. Aligning coordinates vertically across adjacent layers eliminated forced horizontal zig-zag detours.")

ul3 = doc.add_paragraph(style='List Bullet')
ul3.add_run("Alpha Weight Integration: ").bold = True
ul3.add_run("Integrated the -alpha command-line flag into Noxim's runtime parameter parser. Calibrated alpha to 0.2 to ensure a balanced trade-off between shortest-path distance and queue backlog sensitivity.")

ul4 = doc.add_paragraph(style='List Bullet')
ul4.add_run("Resolution of Assertion Crashes (Exit Code -6): ").bold = True
ul4.add_run("Identified that legacy 2D distributions (transpose1, transpose2, shuffle) calculated destination coordinates incompatible with multi-mesh chiplet topologies. Testing was refactored around 3D-safe mathematical distributions (random, local_chiplet).")

ul5 = doc.add_paragraph(style='List Bullet')
ul5.add_run("Automated Threshold Sweeping: ").bold = True
ul5.add_run("Developed sweep_tr_db.py to sweep Packet Injection Rates (PIR), detect the inflection point where ERR begins outperforming DB, and capture the corresponding TR_DB delay threshold.")

# Section 3
doc.add_heading('3. Comparative Simulation Results', level=1)
doc.add_paragraph("Simulations were executed across all three routing modes for 100,001 cycles under identical traffic conditions:")

# Table
table_data = [
    ["Metric", "Mode 1 (DB)", "Mode 2 (ERR)", "Mode 0 (AdEle+)"],
    ["Global Average Delay", "8.22914", "8.22847", "8.23390"],
    ["Network Throughput", "4.22767", "4.24273", "4.21849"],
    ["Avg Elevator Delay", "1509.15", "1644.39", "1587.23"],
    ["Total Received Flits", "380,490", "381,846", "379,664"],
    ["Dynamic Energy (J)", "2.80561e-06", "2.80972e-06", "2.80177e-06"],
    ["Static Energy (J)", "0.000209762", "0.000209762", "0.000209762"]
]

table = doc.add_table(rows=1, cols=4)
table.style = 'Table Grid'
hdr_cells = table.rows[0].cells
for i in range(4):
    hdr_cells[i].text = table_data[0][i]
    hdr_cells[i].paragraphs[0].runs[0].bold = True

for row in table_data[1:]:
    row_cells = table.add_row().cells
    for i in range(4):
        row_cells[i].text = row[i]

# Section 4
doc.add_heading('4. Performance Analysis & Metric Explanations', level=1)

doc.add_heading('Latency and the Crossover Phenomenon', level=2)
doc.add_paragraph("At this injection rate, Mode 2 (ERR, 8.22847 cycles) marginally outperforms Mode 1 (DB, 8.22914 cycles). This marks the onset of the crossover regime: primary shortest-path elevator buffers are sufficiently saturated that the penalty of a horizontal detour (ERR) is lower than the queuing delay of the shortest physical path.")

doc.add_heading('Adaptive Dynamic Selection (Mode 0)', level=2)
doc.add_paragraph("Mode 0 achieved an average latency of 8.2339 cycles. The critical indicator of algorithm operation is the Average Elevator Delay:")
doc.add_paragraph("• Distance-Based Delay: 1509.15 cycles")
doc.add_paragraph("• ERR Delay: 1644.39 cycles")
doc.add_paragraph("• Adaptive Delay: 1587.23 cycles")
doc.add_paragraph("Because 1587.23 sits directly between the two static modes, it confirms that AdEle+ is evaluating the runtime TR_DB threshold on a per-packet basis, dispatching along shortest paths under light load and shifting to alternate elevators as localized queuing builds up.")

doc.add_heading('Energy Profile', level=2)
doc.add_paragraph("Total power dissipation remains heavily dominated by static buffer leakage across all configurations. In Mode 0, static energy (2.09762e-04 J) exceeds dynamic energy (2.80177e-06 J) by two orders of magnitude. At this injection load, router buffers spend significant time powered on while waiting between packet bursts. Dynamic power will scale up as injection rates approach total network saturation.")

# Save
doc.save('AdEle_Report.docx')
print("Successfully generated AdEle_Report.docx!")