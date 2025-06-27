-- Little help to get context menus, dialogs, etc in the right place...
require "stack"

local RenderStack = Stack.New()

local function Register(DrawFn)
   if (DrawFn) then
      Stack.Push(RenderStack, DrawFn)
   end
end

local function Flush()
   Stack.IterateRem(RenderStack, function(Item) Item() end)
end

DeferredRender = {
   Register = Register,
   Flush = Flush
}
