#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
跨单位温差计算器 - Cross-Unit Temperature Difference Calculator
A simple GUI tool for comparing temperatures in different units.
"""

import tkinter as tk
from tkinter import ttk
import platform


# ====== Unit symbol mappings ======

SYMBOLS = {
    'zh': {'C': '\u2103', 'F': '\u2109', 'K': 'K'},
    'en': {'C': '\u00b0C', 'F': '\u00b0F', 'K': 'K'},
    'ascii': {'C': '_C', 'F': '_F', 'K': '_K'},
}


# ====== Temperature Conversion Functions ======

def to_celsius(value, unit):
    """Convert value from given unit to Celsius."""
    if unit == 'C':
        return value
    elif unit == 'F':
        return (value - 32.0) * 5.0 / 9.0
    elif unit == 'K':
        return value - 273.15


def format_temp_line(value, unit_char, tag, ascii_mode):
    """Format one temperature line with unit symbol and optional tag."""
    mode = 'ascii' if ascii_mode else 'en'
    sym = SYMBOLS[mode][unit_char]
    return f"{value:.1f} {sym}{tag}"


def format_diff_value(value, unit_char, ascii_mode):
    """Format a single difference value with unit."""
    mode = 'ascii' if ascii_mode else 'en'
    sym = SYMBOLS[mode][unit_char]
    return f"{value:.1f} {sym}"


# ====== GUI Application ======

class App(tk.Tk):
    UNITS = ('C', 'F', 'K')
    UNIT_DISPLAY = {'C': '\u2103', 'F': '\u2109', 'K': 'K'}

    def __init__(self):
        super().__init__()
        self.title("\u8de8\u5355\u4f4d\u6e29\u5dee\u8ba1\u7b97\u5668")
        self.geometry("280x395")
        self.resizable(False, False)
        self.attributes("-topmost", True)

        # -- State --
        self.lang = tk.StringVar(value='zh')
        self.ascii_only = tk.BooleanVar(value=False)
        self._calculated = False

        # Unit indices for each input row (0=C, 1=F, 2=K)
        self._unit_idx1 = tk.IntVar(value=0)
        self._unit_idx2 = tk.IntVar(value=0)

        # -- Build UI --
        self._create_widgets()
        self._layout_widgets()
        self._show_startup_text()
        self._minimize_console()

        # -- Trace language changes --
        self.lang.trace_add('write', self._on_lang_change)
        self.ascii_only.trace_add('write', self._on_lang_change)

    # ------------------------------------------------------------
    # Widget creation
    # ------------------------------------------------------------
    def _create_widgets(self):
        self._setup_inputs()
        self._setup_calc_button()
        self._setup_language_options()
        self._setup_result_area()

    def _setup_inputs(self):
        vcmd = (self.register(self._validate_number), '%P')

        self.entry1 = ttk.Entry(self, validate='key', validatecommand=vcmd)
        self.entry2 = ttk.Entry(self, validate='key', validatecommand=vcmd)

        self.unit_btn1 = ttk.Button(
            self, text=self.UNIT_DISPLAY[self.UNITS[0]],
            command=lambda: self._cycle_unit(1)
        )
        self.unit_btn2 = ttk.Button(
            self, text=self.UNIT_DISPLAY[self.UNITS[0]],
            command=lambda: self._cycle_unit(2)
        )

        # Enter key chain: entry1 -> entry2 -> calculate
        self.entry1.bind('<Return>', lambda e: self.entry2.focus())
        self.entry2.bind('<Return>', lambda e: self.calculate())

    def _setup_calc_button(self):
        self.calc_btn = ttk.Button(
            self, text="\u8fd0\u7b97",
            command=self.calculate
        )

    def _setup_language_options(self):
        self.lang_frame = ttk.Frame(self)
        self.rb_zh = ttk.Radiobutton(
            self.lang_frame, text="\u4e2d\u6587",
            variable=self.lang, value='zh'
        )
        self.rb_en = ttk.Radiobutton(
            self.lang_frame, text="EN",
            variable=self.lang, value='en'
        )
        self.cb_ascii = ttk.Checkbutton(
            self.lang_frame, text="ASCII ONLY",
            variable=self.ascii_only
        )
        self.rb_zh.pack(side='left', padx=(0, 2))
        self.rb_en.pack(side='left', padx=(0, 8))
        self.cb_ascii.pack(side='left')

    def _setup_result_area(self):
        self.result_text = tk.Text(
            self, wrap='word', width=30, height=10,
            font=('Consolas', 10)
        )
        self.result_text.config(state='disabled')

    # ------------------------------------------------------------
    # Layout
    # ------------------------------------------------------------
    def _layout_widgets(self):
        # Column 0: entry (75%), Column 1: unit button (25%)
        self.grid_columnconfigure(0, weight=3, uniform='input')
        self.grid_columnconfigure(1, weight=1, uniform='input')
        # Row 5: result area expands
        self.grid_rowconfigure(5, weight=1)

        # Row 0: Input 1
        self.entry1.grid(row=0, column=0, sticky='ew',
                         padx=(4, 1), pady=(10, 2))
        self.unit_btn1.grid(row=0, column=1, sticky='ew',
                            padx=(1, 4), pady=(10, 2))

        # Row 1: Input 2
        self.entry2.grid(row=1, column=0, sticky='ew',
                         padx=(4, 1), pady=(2, 2))
        self.unit_btn2.grid(row=1, column=1, sticky='ew',
                            padx=(1, 4), pady=(2, 2))

        # Row 2: Calculate button (full width)
        self.calc_btn.grid(row=2, column=0, columnspan=2,
                           sticky='ew', padx=4, pady=(8, 4))

        # Row 3: Language options
        self.lang_frame.grid(row=3, column=0, columnspan=2,
                             sticky='w', padx=4, pady=(0, 4))

        # Row 4: Separator
        sep = ttk.Separator(self, orient='horizontal')
        sep.grid(row=4, column=0, columnspan=2, sticky='ew',
                 padx=4, pady=(0, 4))

        # Row 5: Result text area (fills remaining space)
        self.result_text.grid(row=5, column=0, columnspan=2,
                              sticky='nsew', padx=4, pady=(0, 4))

    # ------------------------------------------------------------
    # Input validation
    # ------------------------------------------------------------
    def _validate_number(self, value):
        if value == '':
            return True
        # Only digits and at most one decimal point
        if value.count('.') > 1:
            return False
        allowed = set('0123456789.-')
        if not all(c in allowed for c in value):
            return False
        # minus sign only at the start
        if value.count('-') > 1 or (value.count('-') == 1 and value[0] != '-'):
            return False
        # '.' not alone
        if value == '.' or value == '-.':
            return False
        return True

    # ------------------------------------------------------------
    # Cycle unit button
    # ------------------------------------------------------------
    def _cycle_unit(self, row):
        var = self._unit_idx1 if row == 1 else self._unit_idx2
        btn = self.unit_btn1 if row == 1 else self.unit_btn2
        var.set((var.get() + 1) % 3)
        btn.config(text=self.UNIT_DISPLAY[self.UNITS[var.get()]])

    # ------------------------------------------------------------
    # Calculate
    # ------------------------------------------------------------
    def calculate(self):
        # Get input values
        try:
            v1_str = self.entry1.get().strip()
            v2_str = self.entry2.get().strip()
            if not v1_str or not v2_str:
                msg = ("\u8bf7\u8f93\u5165\u4e24\u4e2a\u6e29\u5ea6\u503c"
                       if self.lang.get() == 'zh' else
                       "Please enter both temperatures")
                self._show_result(msg)
                return
            v1 = float(v1_str)
            v2 = float(v2_str)
        except ValueError:
            msg = ("\u8bf7\u8f93\u5165\u6709\u6548\u7684\u6570\u5b57"
                   if self.lang.get() == 'zh' else
                   "Please enter valid numbers")
            self._show_result(msg)
            return

        u1_char = self.UNITS[self._unit_idx1.get()]
        u2_char = self.UNITS[self._unit_idx2.get()]

        # Convert both to Celsius for comparison
        c1 = to_celsius(v1, u1_char)
        c2 = to_celsius(v2, u2_char)

        is_ascii = self.ascii_only.get()
        is_zh = self.lang.get() == 'zh'

        eps = 1e-10

        # Determine tags
        if abs(c1 - c2) < eps:
            tag1 = ""
            tag2 = ""
        elif c1 > c2:
            tag1 = "\uff08\u66f4\u5927\uff09" if is_zh else " (Bigger)"
            tag2 = ""
        else:
            tag1 = ""
            tag2 = "\uff08\u66f4\u5927\uff09" if is_zh else " (Bigger)"

        # Format lines
        line1 = format_temp_line(v1, u1_char, tag1, is_ascii)
        line2 = format_temp_line(v2, u2_char, tag2, is_ascii)

        # Difference
        diff_c = abs(c1 - c2)
        diff_f = diff_c * 9.0 / 5.0
        diff_k = diff_c

        if is_zh:
            diff_label = "\u5dee\u503c\uff1a"
        elif is_ascii:
            diff_label = "Diff:"
        else:
            diff_label = "Difference:"

        parts = [
            format_diff_value(diff_c, 'C', is_ascii),
            format_diff_value(diff_f, 'F', is_ascii),
            format_diff_value(diff_k, 'K', is_ascii),
        ]
        diff_line = " / ".join(parts)

        output = f"{line1}\n{line2}\n\n{diff_label}\n{diff_line}"

        self._show_result(output)
        self._calculated = True

    # ------------------------------------------------------------
    # Show result / startup text
    # ------------------------------------------------------------
    def _show_result(self, text):
        self.result_text.config(state='normal')
        self.result_text.delete('1.0', 'end')
        self.result_text.insert('1.0', text)
        self.result_text.config(state='disabled')

    def _show_startup_text(self):
        is_zh = self.lang.get() == 'zh'
        is_ascii = self.ascii_only.get()

        if is_zh:
            text = (
                "\u8de8\u5355\u4f4d\u6e29\u5dee\u8ba1\u7b97\u5668 demo\n\n"
                "\u5728\u4e0a\u65b9\u8f93\u5165\u4e24\u4e2a\u6e29\u5ea6\uff0c\n"
                "\u6309\u4e0b\u6309\u94ae\u5207\u6362\u5355\u4f4d\u3002\n\n"
                "\u6309\u4e0b\u8fd0\u7b97\u540e\uff0c\n"
                "\u5c06\u5728\u6b64\u5c55\u793a\u54ea\u4e2a\u66f4\u5927\uff0c\n"
                "\u4ee5\u53ca\u4e09\u4e2a\u5355\u4f4d\u7684\u6e29\u5dee\u3002"
            )
        else:
            text = (
                "Cross-Unit Temperature Difference Demo\n\n"
                "Enter two temperatures above.\n"
                "Click the unit button to switch.\n\n"
                "Press Calculate to see\n"
                "which one is bigger and the\n"
                "difference in all three units."
            )

        self._show_result(text)

    def _on_lang_change(self, *_args):
        # If no calculation result shown, refresh startup text
        if not self._calculated:
            self._show_startup_text()
        else:
            self._calculated = False
            self._show_startup_text()

        # Update calc button text
        if self.lang.get() == 'zh':
            self.calc_btn.config(text="\u8fd0\u7b97")
        else:
            self.calc_btn.config(text="Calculate")

    # ------------------------------------------------------------
    # Utility
    # ------------------------------------------------------------
    def _minimize_console(self):
        if platform.system() == "Windows":
            try:
                import ctypes
                hwnd = ctypes.windll.kernel32.GetConsoleWindow()
                if hwnd:
                    ctypes.windll.user32.ShowWindow(hwnd, 6)
            except Exception:
                pass


# ====== Entry Point ======
if __name__ == "__main__":
    App().mainloop()
