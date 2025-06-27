require "ui_base"
require "ui_preferences"
require "math"

local NormalBackGrid   = NineGrid_GetHandle("slider_bg")
local NormalPuckUp   = UIBitmap_GetHandle("slider_circle_rest")
local NormalPuckDown = UIBitmap_GetHandle("slider_circle_pressed")

local SmallBackGrid = NineGrid_GetHandle("slider_bg_small")
local SmallPuckUp   = UIBitmap_GetHandle("slider_circle_rest_small")
local SmallPuckDown = UIBitmap_GetHandle("slider_circle_pressed_small")

local PuckW, PuckH = UIBitmap_Dimensions(PuckUp)

local function MakeSlide(Item)
   local function SlideCoroutine(Event)
      Event = WaitForEvent(Event, LButtonDown, false)

      Item.IsDragging = true
      Item.DragVal = Item.Val

      Event = WaitForEvent(Event, LButtonUp, true,
                           function (Event)
                              if (Event.Point) then
                                 local Interp  = (Event.Point.x - Item.Rect.x - PuckW/2)
                                 local NewFrac = Interp / Item.AllowedRange

                                 if (NewFrac < 0) then NewFrac = 0 end
                                 if (NewFrac > 1) then NewFrac = 1 end

                                 Item.DragVal = Item.Min + NewFrac * (Item.Max - Item.Min)
                              end
                           end)

      Item.Val         = Item.DragVal
      Item.IsDragging  = false
      Item.SlideAction = nil
   end

   return { Action = coroutine.create(SlideCoroutine),
            Abort  = function ()
                        Item.IsDragging = false
                        Item.SlideAction = nil
                     end }
end

local function DoInternal(ID, Val, Min, Max, BackGrid, PuckUp, PuckDown)
   local Item, IsNew = IDItem.Get(ID)

   Item.Rect = UIArea.Get()

   if (not Item.IsDragging) then
      Item.Val  = Val
      Item.Min  = Min
      Item.Max  = Max
   end

   Item.AllowedRange = Item.Rect.w - PuckW

   local UseVal = Val
   local PuckHandle = PuckUp
   if (Item.IsDragging) then
      UseVal = Item.DragVal
      PuckHandle = PuckDown
   end

   local CenterParam = (UseVal - Min) / (Max - Min)
   local CenterX     = math.floor(Item.Rect.x + Item.AllowedRange * CenterParam)
   local CenterY     = Item.Rect.y + (Item.Rect.h / 2)

   local PuckY = CenterY - PuckH/2
   local BGY   = CenterY - 3

   NineGrid_Draw(BackGrid, MakeRect(Item.Rect.x, BGY, Item.Rect.w, 7))
   UIBitmap_Draw(PuckHandle, MakePoint(CenterX, PuckY))

   if (not Interactions.Active()) then
      local InteractRect = MakeRect(CenterX, PuckY, PuckW, PuckH)
      Item.SlideAction = Interactions.RegisterDefClipped(Item.SlideAction or MakeSlide(Item))
   end

   return UseVal, (Item.IsDragging == true)
end


Slider = {
   Do      = function(ID, Val, Min, Max) return DoInternal(ID, Val, Min, Max, NormalBackGrid, NormalPuckUp, NormalPuckDown) end,
   DoSmall = function(ID, Val, Min, Max) return DoInternal(ID, Val, Min, Max, SmallBackGrid,  SmallPuckUp,  SmallPuckDown)  end
}

