from chess import *
from flask import Flask, render_template, redirect, session, url_for, request
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO, join_room
from dataclasses import dataclass
from typing import Dict, Set, Deque, List
from string import ascii_uppercase
import random
from pympler import asizeof

@dataclass
class GameData:
    white : str
    id : str


class ChessApp(Flask):
    def __init__(self):
        super().__init__(__name__)
        self.secret_key = "123456"
        self._waiting : Deque[GameData] = deque()
        self._games : Dict[str, Game] = {}
        self._taken_ids :Set[str] = set()
        self._playerdata : Dict[str,str] = {}

    def gen_unique(self):
        while True:
            temp = ""
            for _ in range(8):
                temp += random.choice(ascii_uppercase)
            if temp not in self._taken_ids:
                return temp

    def add_game(self, player : str):
        print(self._waiting, self._games)
        if self._waiting:
            gamedata : GameData = self._waiting.popleft()
            game = self._games[gamedata.id]
            game.black = player
            self._playerdata[player] = gamedata.id
            return 200
        new_code = self.gen_unique()
        new_game = GameData(white=player, id = new_code)
        game = Game()
        game.white = player
        self._playerdata[player]=new_code
        self._waiting.append(new_game)
        self._games[new_code] = game
        return 200


app = ChessApp()
socket = SocketIO(app=app)

def send_board(game,id):
    print(asizeof.asizeof(int.from_bytes(game.board)))
    socket.emit('renderboard',int.from_bytes(game.board), to=id)

@app.route('/')
def index():
    if not (name := session.get("name")):
        return redirect(url_for('login'))
    return render_template('index.html')

@app.route('/login', methods = ["GET", "POST"])
def login():
    session.clear()
    if request.method == "POST":
        name = request.form.get("name")
        session["name"]=name
        app.add_game(name)
        return redirect(url_for('index'))
    return render_template('login.html')

@socket.on('connect')
def connected():
    if not (name:=session.get("name")):
        return 404
    if name not in app._playerdata:
        return 404
    id = app._playerdata[name]
    join_room(id)
    socket.sleep(0.1)
    print('connected user')
    game = app._games[id]
    send_board(game,id)

@socket.on('move')
def newmove(data : List[str]):
    print(data)
    if not (name:=session.get("name")):
        print('nameerror')
        return 404
    if name not in app._playerdata:
        print('dicterror')
        return 404
    from_square, to_square = data
    id = app._playerdata[name]
    game : Game= app._games[id]
    game.move(from_square, to_square)
    print(game)
    send_board(game,id)

if __name__ == '__main__':
    socket.run(app=app, port= 8080, debug=True)
