from flask import Blueprint

users_bp = Blueprint('users', __name__)

@users_bp.route('/login')
def login():
    return "This is the login page."

@users_bp.route('/register')
def register():
    return "This is the register page."