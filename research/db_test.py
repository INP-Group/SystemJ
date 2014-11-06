from PyDbLite import Base

db = Base()
db.create("name", "age", "favourite_color")

# You can insert records as either named parameters
# or in the order of the fields
db.insert(name="Joe", age=16, favourite_color=None)
db.insert("Jane", None, "red")

# These should return an object you can iterate over
# to get the matching records.  These are unindexed queries.
#
# The first might throw because of the None in the second record
over_16 = db("age") > 16
with_favourite_colors = db("favourite_color") != None

# Or you can make an index for faster queries
db.create_index("favourite_color")
with_favourite_color_red = db._favourite_color["red"]