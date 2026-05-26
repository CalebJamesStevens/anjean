I'm thinking of something like this for the framework:
Essentially we need a way for dev to define an entry point for a discrete
section of a game. That entry point will be responsible for returning an object that defined the seciton.

In web development you might use a framework built around the React library. It's common to have a 
portion of your applicaiton dedicated to defining the routes of your application and what entry point that route
points to. When a user hits an endpoint, the server will forward the request to the first matching rule. This enpoint will run the
code described and return the data for the browser to display. 

```jsx
import  HomePage from 'HomePage.jsx'
import  AboutUsPage from 'AboutUsPage.jsx'

function App = () => {
  return (
    <>
      <Route path='/' element={HomePage} />
      <Route path='/about-us' element={AboutUsPage} />
    </>
  )
}
```

This is an example of a rudementary routing sytem in web development. Internally the Route element will check if the current
path of the browser is equal to its own, and if so, it will return the element passed to it. That element will then be caleld on
and it will execute, eventually returning it's own data to be display.

It could be interesting to have a game engine that flows similarly to this convention in the code itself.

For example:

```
Game app starts -> runs code at given entry point -> creates renderer (2d, 3d, etc) based on entry point return data type.
```