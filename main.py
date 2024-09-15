import curses
import os
import random
import threading
import time

char_choice = [
    "ﾊ","ﾐ", "ﾋ","ｰ","ｳ","ｼ","ﾅ","ﾓ","ﾆ","ｻ","ﾜ","ﾂ","ｵ","ﾘ","ｱ","ﾎ",
    "ﾃ","ﾏ","ｹ","ﾒ","ｴ","ｶ","ｷ","ﾑ","ﾕ","ﾗ","ｾ","ﾈ","ｽ","ﾀ","ﾇ","ﾍ",
    "0","1","2","3","4","5","6","7","8","9","Y","Z",
    ":",".","=","*","+","-","<",">","¦","|"
]

white = 4
black = 5
green = [
    (34, 180, 85),      # green
    (128, 206, 135),    # light green
    (56, 165, 49),      # mid green
    (32, 72, 41),       # dark green
    (255, 255, 255),    # white
    (0, 0, 0),          # black
]
blue = [
    (74, 184, 249),     # bright blue
    (9, 65, 152),       # blue
    (14, 35, 115),      # dark blue
    (9,0,136),          # dark blue
    (255, 255, 255),    # white
    (0, 0, 0),          # black
]
red = [
    (212,0,0),          # red
    (240,57,57),        # bright red
    (148,0,0),          # mid red
    (92,16,16),         # dard red
    (255, 255, 255),    # white
    (0, 0, 0),          # black
]
yellow = [
    (242, 226, 76),
    (189, 171, 8),
    (176, 161, 27),
    (150, 135, 0),
    (255, 255, 255),    # white
    (0, 0, 0),          # black
]

class Drop():
    def __init__(self, x, maximum, source):
        self.x = x
        self.y = 0
        self.height = random.randint(4, int(max(maximum*0.6, 6)))
        self.last = random.choice(char_choice)
        # self.maximum = maximum
        self.source = source
        self.print_char(self.last, self.x, self.y, white, source)
    
    def __del__(self):
        for i in range(self.height):
            self.print_char(" ", self.x, self.y-i, black, self.source)

    def increment(self, source):
        h = os.get_terminal_size().lines
        if (self.y <= h):
            self.print_char(self.last, self.x, self.y, random.randint(0, 3), source)
        self.y += 1
        if (self.y <= h):
            self.last = random.choice(char_choice)
        self.print_char(self.last, self.x, self.y, white, source)
        if (self.y - self.height >= 0):
            self.print_char(" ", self.x, self.y-self.height, black, source)

    def print_char(self, char, x, y, col_value, source):
        # if y < 0 or y > os.get_terminal_size().lines-1: return
        try: source.addstr(y, x, char, curses.color_pair(col_value+1))
        except: return

def init_colors(col, paused, stdscr):
    for i in range(len(col)):
        n = len(col)
        color = [int(1000*col[i%n][0]/255), int(1000*col[i%n][1]/255), int(1000*col[i%n][2]/255)]
        curses.init_color(i+1, color[0], color[1], color[2])
        curses.init_pair(i+1, i+1, curses.COLOR_BLACK)
    if (paused.is_set()): stdscr.refresh()

def input_listener(exit_bool, paused, stdscr):
    while (not exit_bool.is_set()):
        x = input()
        if (x == 'g' or x == 'G'): init_colors(green, paused, stdscr)
        if (x == 'b' or x == 'B'): init_colors(blue, paused, stdscr)
        if (x == 'r' or x == 'R'): init_colors(red, paused, stdscr)
        if (x == 'y' or x == 'Y'): init_colors(yellow, paused, stdscr)
        if (x == ' ' or x == 'p' or x == 'P'):
            if (paused.is_set()): paused.clear()
            else: paused.set()
        if (x == 'e' or x == 'E' or x == 'q' or x == 'Q'):
            paused.clear()
            exit_bool.set()
            return

def display(exit_bool, paused, stdscr):
    curses.curs_set(0)
    stdscr.clear()
    init_colors(green, paused, stdscr)


    size = os.get_terminal_size()
    delta = 15
    neg = -10000

    lastx = [neg]*size.columns
    drops = []

    while (not exit_bool.is_set()):
        if (len(drops) > 1000): exit(0)
        while (paused.is_set()):
            stdscr.refresh()
            time.sleep(0.07)

        size = os.get_terminal_size()

        if (len(lastx) < size.columns):
            lastx = lastx + [neg]*(size.columns - len(lastx))
        for i in range(len(lastx)):
            if ((lastx[i] > delta or lastx[i] == neg) and random.randint(0, 100) == 0):
                drops.append(Drop(i, size.lines/2, stdscr))
                lastx[i] = drops[-1].y - drops[-1].height
        
        for i in range(len(drops) - 1, -1, -1):
            lastx[drops[i].x] += 1
            drops[i].increment(stdscr)
            if (drops[i].y - drops[i].height > size.lines+10+delta):# and
                #drops[i].x < size.columns):
                del drops[i]
                drops.pop(i)
        
        time.sleep(.07)
        stdscr.refresh()

    curses.curs_set(1)
    stdscr.clear()
    stdscr.refresh()

    for drop in drops: del drop
    del drops

def main(stdscr):
    paused = threading.Event()
    exit_program = threading.Event()
    threading.Thread(target=input_listener, args=(exit_program, paused, stdscr,)).start()
    threading.Thread(target=display, args=(exit_program, paused, stdscr,)).start()

if __name__ == "__main__":
    curses.wrapper(main)


