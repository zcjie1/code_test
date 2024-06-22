from flask import Flask
from users import users_bp
from posts import posts_bp

app = Flask(__name__)

# 注册蓝图
app.register_blueprint(users_bp, url_prefix='/users')
app.register_blueprint(posts_bp, url_prefix='/posts')

if __name__ == '__main__':
    app.run(debug=True)