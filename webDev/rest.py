from glob import escape
from flask import Flask, jsonify, request, session
from flask_oauthlib.client import OAuth

app = Flask(__name__) # An instance of this class will be our WSGI application. name is the name of our module

oauth = OAuth(app)


books = [    {        "title": "The Great Gatsby",        "author": "F. Scott Fitzgerald",        "year": 1925, "price" : 10    },    
            {        "title": "To Kill a Mockingbird",        "author": "Harper Lee",        "year": 1960, "price" : 10     },    
            {        "title": "1984",        "author": "George Orwell",        "year": 1949, "price" : 10     }]

"""
OAuth configuration
"""

app.config['SECRET_KEY'] = 'super secret key'
app.config['GOOGLE_ID'] = '<your google client id>'
app.config['GOOGLE_SECRET'] = '<your google client secret>'
app.config['REDIRECT_URI'] = 'http://localhost:5000/google_login/authorized'
app.config['SCOPE'] = 'email'

google = oauth.remote_app(
    'google',
    consumer_key=app.config['GOOGLE_ID'],
    consumer_secret=app.config['GOOGLE_SECRET'],
    request_token_params={
        'scope': app.config['SCOPE']
    },
    base_url='https://www.googleapis.com/oauth2/v1/',
    authorize_url='https://accounts.google.com/o/oauth2/auth',
    request_token_url=None,
    access_token_method='POST',
    access_token_url='https://accounts.google.com/o/oauth2/token',
    access_token_params={'grant_type': 'authorization_code'}
)

@app.route("/") #We then use the route() decorator to tell Flask what URL should trigger our function
def hello_world():
    return "<p>Hello, nick!</p>"

@app.route("/books", methods=['GET']) #By default, Flask routes only respond to GET requests, 
# so if you want to allow other methods like POST, PUT, or DELETE, you need to explicitly specify them using the methods argument.
def get():
    return jsonify({'books': books})

"""
You can add variable sections to a URL by marking sections with <variable_name>.
Optionally, you can use a converter to specify the type of the argument like <converter:variable_name>.
GET:
"""
@app.route("/books/<int:book>", methods=['GET']) 
def get_book(book): 
    return jsonify({"book": books[book]}) # use escape to prevent injection attacks

"""
To post: curl -i -H "Content-Type: Application/json" -X POST http://localhost:5000/books
Curl is a command-line tool used to transfer data to or from a server. It supports many protocols, including HTTP, HTTPS, FTP, SFTP, and more.
We need to use it here 
You can use curl to execute an HTTP request to your Flask application's API endpoint. This is useful for testing your application and making sure it 
is working correctly. By using curl, you can simulate a client making a request to your API endpoint and verify that the response is correct.
In a production environment, you would typically use a client-side application or a server-to-server request to access your API endpoints rather than 
using curl directly.
"""
@app.route("/books", methods=['POST'])
def create():
    book = {"title": "Harry Potter", "author": "JK Rowling", "year": 1997} #hard coded what we post
    books.append(book)
    return jsonify({"created": book})

"""
PUT:
curl -i -H "Content-Type: Application/json" -X PUT http://localhost:5000/books/1
"""
@app.route("/books/<int:book_num>", methods=['PUT'])
def update(book_num):
    books[book_num]["price"] = 15
    return jsonify({"book": books[book_num]})

"""
DELETE:
curl -i -H "Content-Type: Application/json" -X DELETE http://localhost:5000/books/1
"""
@app.route("/books/<int:book_num>", methods=['DELETE'])
def delete(book_num):
    books.remove(books[book_num])
    return jsonify({"result": True})

"""
More OAuth stuff
"""

@app.route('/google_login/authorized')
def authorized():
    resp = google.authorized_response()
    if resp is None:
        return 'Access denied: reason={0} error={1}'.format(
            request.args['error_reason'],
            request.args['error_description']
        )
    session['google_token'] = (resp['access_token'], '')
    me = google.get('userinfo')
    return jsonify({"user": me.data})


@google.tokengetter
def get_google_oauth_token():
    return session.get('google_token')
    
if __name__ == "__main__":
    app.run(debug=True)