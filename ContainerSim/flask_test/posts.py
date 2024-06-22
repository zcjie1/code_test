from flask import Blueprint

posts_bp = Blueprint('posts', __name__)

@posts_bp.route('/post')
def post():
    return "This is the post page."