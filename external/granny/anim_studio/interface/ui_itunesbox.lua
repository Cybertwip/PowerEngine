require "deferred_rendering"
require "id_item_cache"
require "ui_area"
require "ui_preferences"

local CBBitmapW, CBBitmapH   = UIBitmap_Dimensions(UIBitmap_GetHandle("combobox"))
local TriBitmapW, TriBitmapH = UIBitmap_Dimensions(UIBitmap_GetHandle("combobox_arrow_pressed"))

local FontHandle    = UIPref.GetFont("combobox")
local FontH, FontBL = GetFontHeightAndBaseline(FontHandle)
local FontY         = FontBL + 5
local FontX         = 10
local HighColor     = UIPref.GetColor("combo_highlight")

local function EntriesRect(InitialX, InitialY, InitialW, InitialH, Entries)
   local Width = InitialW
   for i,str in ipairs(Entries) do
      local w = GetSimpleTextDimension(FontHandle, str) + 2*FontX
      Width = math.max(w, Width)
   end

   -- Needs to adjust to screen position, but always left for now...
   InitialX = InitialX - (Width - InitialW)
   return MakeRect(InitialX, InitialY, Width, InitialH)
end

local function FilterEntriesRect(InitialX, InitialY, InitialW, InitialH, FilterEntries)
   local Width = InitialW
   for i,v in ipairs(FilterEntries) do
      local str = v[1]
      local w = GetSimpleTextDimension(FontHandle, str) + 2*FontX
      Width = math.max(w, Width)
   end

   -- Needs to adjust to screen position, but always left for now...
   InitialX = InitialX - (Width - InitialW)
   return MakeRect(InitialX, InitialY, Width, InitialH)
end


function ITunesBoxDropHandler(Event, Item)
   Item.HighlightEntry = Item.CurrEntry
   Item.Dragging = false
   Item.Dropped = true

   -- todo: compute rect above for list exceeding screensize
   -- todo: prevent clipping
   local EntriesHeight = #Item.Entries * FontH

   Item.EntriesRect = EntriesRect(Item.FullArea.x, Item.FullArea.y + Item.FullArea.h,
                                  Item.FullArea.w, EntriesHeight, Item.Entries)

   Event = WaitForEvent(Event, LButtonUp, true,
                        function (Event)
                           if (Event.Point) then
                              if (RectContainsPoint(Item.EntriesRect, Event.Point)) then
                                 Item.Dragging = true
                                 Item.HighlightEntry = math.floor((Event.Point.y - Item.EntriesRect.y) / FontH) + 1
                              end
                           end
                        end)

   if (not Item.Dragging) then
      Event = WaitForEvent(Event, LButtonDown, true,
                           function (Event)
                              if (Event.Point) then
                                 if (RectContainsPoint(Item.EntriesRect, Event.Point)) then
                                    Item.Dragging = true
                                    Item.HighlightEntry = math.floor((Event.Point.y - Item.EntriesRect.y) / FontH) + 1
                                 end
                              end
                           end)
   end

   if (Event.Point) then
      if (RectContainsPoint(Item.EntriesRect, Event.Point)) then
         Item.CurrEntry = Item.HighlightEntry
         Item.IndexChanged = true
      end
   end

   Item.Dragging = false
   Item.Dropped = false
end

local function MatchText(test_str, comp_strs)
   for i,v in ipairs(comp_strs) do
      if (string.find(string.lower(test_str), v, 1, true) == nil) then
         return false
      end
   end

   return true
end

local function GetFilters(Item)
   -- Parse out the comp strs
   local comp_strs = {}

   if (Item.STBEdit) then
      local Text, SelStart, SelEnd, Cursor = STB_GetTextParams(Item.STBEdit)
      for v in string.gmatch(Text, "([^ \t]+)") do
         table.insert(comp_strs, string.lower(v))
      end
   end

   return comp_strs
end

function ITunesBoxDrawEntryList(Item)
   if (Item.Dropped or Item.ITunesDropped) then
      -- todo: drop shadow?

      local Capture = UIAreaCapture()
      DeferredRender.Register(
         function ()
            local UseRect
            if (Item.ITunesDropped) then
               UseRect    = Item.FilteredRect
               UseEntries = Item.FilteredEntries
            else
               UseRect    = Item.EntriesRect
               UseEntries = Item.Entries
            end

            local UL, Clipped = UIAreaTransformFromCapture(Capture, MakePoint(UseRect.x, UseRect.y))
            local Rect = MakeRect(UL.x, UL.y,
                                  UseRect.w, UseRect.h + 5)

            NineGrid_Draw(NineGrid_GetHandle("combobox_menu"), Rect)

            -- weird, but lines up correctly
            Rect.x = Rect.x + FontX
            Rect.w = Rect.w - FontX - 4

            UIArea.ProtectedSimplePush(
               Rect,
               function()
                  local CurrY = 0

                  -- todo: last highlight rect missing a pixel at the bottom
                  for i,item in ipairs(UseEntries) do
                     local str = item
                     if (type(item) ~= "string") then
                        str = item[1]
                     end

                     local TextRect = MakeRect(0, CurrY, Rect.w, FontH)

                     local RectColor = nil
                     local TextColor = UIPref.GetColor("combo_text")

                     if (i == Item.HighlightEntry and Item.Dragging) then
                        RectColor = HighColor
                        TextColor = UIPref.GetColor("combo_highlighttext")
                     elseif (Item.FilteredHighlightEntry and i == Item.FilteredHighlightEntry and Item.ITunesDropped) then
                        RectColor = HighColor
                        TextColor = UIPref.GetColor("combo_highlighttext")
                     end

                     if (RectColor) then
                        local HighRect = MakeRect(TextRect.x-3, TextRect.y, TextRect.w+5, FontH)
                        DrawRect(HighRect, HighColor)
                     end

                     RenderText(FontHandle, str, 0, MakePoint(0, CurrY + FontBL), TextColor)

                     CurrY = CurrY + FontH
                  end
               end)
         end)
   end
end


local function MakeClick(Item)
   local function ClickCoroutine(Event)
      Event = WaitForEvent(Event, LButtonDown, false)

      ITunesBoxDropHandler(Event, Item)

      Item.ClickAction = nil
   end

   return { Action = coroutine.create(ClickCoroutine),
            Abort  = function ()
                        Item.Dragging = false
                        Item.Dropped = false
                        Item.ButtonDown = false
                     end }
end


local function RenderTextWhileEditing(Item, Point)
   local Text, SelStart, SelEnd, Cursor = STB_GetTextParams(Item.STBEdit)

   -- Actual text
   RenderDropText(FontHandle, Text, 0, MakePoint(0, FontY),
                  UIPref.GetColor("combo_text"),
                  UIPref.GetColor("combo_text_drop"))

   -- highlight
   if (SelEnd > SelStart) then
      UIArea.ProtectedSimplePush(
         MakeRect(SelStart, 0, SelEnd - SelStart, UIAreaGet().h),
         function()
            DrawRect(UIAreaGet(), MakeColor(0, 0, 1))
            RenderDropText(FontHandle, Text, 0, MakePoint(-SelStart, FontY),
                           UIPref.GetColor("combo_text"),
                           UIPref.GetColor("combo_text_drop"))
         end)
   end

   -- Cursor
   DrawRect(MakeRect(Cursor-1, 0, 2, Item.FontHeight), MakeColor(1, 1, 1))
end

local function FiltersDifferent(one, two)
   if (#one ~= #two) then
      return true
   end

   for i,str in ipairs(one) do
      if (str ~= two[i]) then
         return true
      end
   end

   return false
end

local function FilterEntries(Item)
   local comp_strs = GetFilters(Item)

   Item.FilteredEntries = { }
   for i,str in ipairs(Item.Entries) do
      if (MatchText(str, comp_strs)) then
         table.insert(Item.FilteredEntries, { str, i })
      end
   end

   local EntriesHeight = #Item.FilteredEntries * FontH
   Item.FilteredRect = FilterEntriesRect(Item.FullArea.x, Item.FullArea.y + Item.FullArea.h,
                                         Item.FullArea.w, EntriesHeight, Item.FilteredEntries)

   -- Check to see if this has changed
   if (not Item.FilterApplied or FiltersDifferent(comp_strs, Item.FilterApplied)) then
      Item.FilteredHighlightEntry = nil
      Item.FilterApplied = comp_strs
   end
end

local function ITunesStart(Item)
   local EntriesHeight = #Item.Entries * FontH
   Item.EntriesRect = EntriesRect(Item.FullArea.x, Item.FullArea.y + Item.FullArea.h,
                                  Item.FullArea.w, EntriesHeight, Item.Entries)
   Item.ITunesDropped = true
   FilterEntries(Item)

   STB_SelectAllPosEnd(Item.STBEdit)
end

local function ITunesEnd(Success, Item)
   Item.ITunesDropped = false

   if (not Success) then
      -- Unchanged
      -- Item.IndexChanged = NewEntry ~= Item.CurrEntry
      -- Item.CurrEntry = NewEntry
      -- Item.Canonical = Item.Entries[NewEntry]

      -- Clear
      Item.FilteredEntries = nil
      Item.FilteredHighlightEntry = nil
      Item.FilteredHighlightEntrySetByMouse = nil
      Item.FilterApplied = nil
      return
   end

   if (Item.FilteredHighlightEntry ~= nil) then
      local NewEntry = Item.FilteredEntries[Item.FilteredHighlightEntry][2]

      Item.IndexChanged = NewEntry ~= Item.CurrEntry
      Item.CurrEntry = NewEntry
      Item.Canonical = Item.Entries[NewEntry]

      Item.FilteredEntries = nil
      Item.FilteredHighlightEntry = nil
      Item.FilteredHighlightEntrySetByMouse = nil
      Item.FilterApplied = nil
   else
      Item.FilteredEntries = nil
      Item.FilteredHighlightEntry = nil
      Item.FilteredHighlightEntrySetByMouse = nil
      Item.FilterApplied = nil

      -- Pick the first item that matches the filter, unless there aren't any
      local comp_strs = GetFilters(Item)
      if (#comp_strs == 0) then
         return
      end

      for i,str in ipairs(Item.Entries) do
         if (MatchText(str, comp_strs)) then
            Item.IndexChanged = i ~= Item.CurrEntry
            Item.CurrEntry = i
            Item.Canonical = str
            return
         end
      end
   end
end

local function ITunesEvent(Item, Event)
   -- return value is (CancelEdit, IgnoreEvent)
   FilterEntries(Item)

   -- If this is a mouse event, set the highlight entry...
   if (Event.Point) then
      if (RectContainsPoint(Item.FilteredRect, Event.Point)) then
         Item.FilteredHighlightEntry = math.floor((Event.Point.y - Item.FilteredRect.y) / FontH) + 1
         Item.FilteredHighlightEntrySetByMouse = true
      else
         -- Yeah?  Classy!
         if (Item.FilteredHighlightEntrySetByMouse) then
            Item.FilteredHighlightEntry = nil
            Item.FilteredHighlightEntrySetByMouse = false
         end
      end

      if (Event.Type == LButtonDown) then
         return true, RectContainsPoint(Item.FilteredRect, Event.Point)
      end
   end

   if (Event.Type == DirKeyUp and (Event.Key == Direction_KeyUp or Event.Key == Direction_KeyDown)) then
      return false, true
   end

   if (Event.Type ~= DirKeyDown) then
      return false, false
   end

   local Inc = nil
   if (Event.Key == Direction_KeyUp) then
      Inc = -1
   elseif (Event.Key == Direction_KeyDown) then
      Inc = 1
   end

   if (not Inc or #Item.FilteredEntries == 0) then
      return false, false
   end

   if (Item.FilteredHighlightEntry) then
      Item.FilteredHighlightEntry = Item.FilteredHighlightEntry + Inc
   else
      Item.FilteredHighlightEntry = 1
   end

   if (Item.FilteredHighlightEntry <= 0) then
      Item.FilteredHighlightEntry = 1
   elseif (Item.FilteredHighlightEntry > #Item.FilteredEntries) then
      Item.FilteredHighlightEntry = #Item.FilteredEntries
   end

   -- Something touched it here...
   Item.FilteredHighlightEntrySetByMouse = false

   STB_SelectAllPosEnd(Item.STBEdit)
   return false, true
end

local function DoITunesBased(ID, Entries, CurrEntry)
   local Item, IsNew = IDItem.Get(ID)
   if (IsNew) then
      Item.Dropped = false
      Item.IndexChanged = false
      Item.Changed = nil

      Item.Editing = false
      Item.FontHandle = FontHandle
      Item.FontHeight, Item.FontBaseline = GetFontHeightAndBaseline(FontHandle)
   end

   if (not Item.Dropped and not Item.IndexChanged) then
      Item.CurrEntry = CurrEntry
   end

   Item.FullArea = UIArea.Get()
   Item.Entries = Entries

   local ButtonArea  = MakeRect(Item.FullArea.x + Item.FullArea.w - TriBitmapW, 0,
                                TriBitmapW, TriBitmapH)
   local DisplayArea = MakeRect(0, 0, Item.FullArea.w - TriBitmapW, Item.FullArea.h)
   local TextArea    = MakeRect(FontX, 0, Item.FullArea.w - TriBitmapW - FontX, Item.FullArea.h)

   -- Register the interaction, recreating if necessary
   if (not Interactions.Active() and #Entries ~= 0) then
      Item.ClickAction = Item.ClickAction or MakeClick(Item)

      UIArea.ProtectedSimplePush(
         TextArea,
         function()
            local Area = UIArea.Get()
            Item.Canonical = Entries[Item.CurrEntry]

            -- Register the interaction
            if (not Interactions.Active()) then
               Item.EditAction =
                  Interactions.RegisterDefClipped(Item.EditAction or
                                                  MakeEdit{Item = Item,
                                                           Rect = Area,
                                                           IgnoreFirstClick = true,
                                                           StartCallback = ITunesStart,
                                                           EndCallback   = ITunesEnd,
                                                           EventCallback = ITunesEvent})
            end

         end)

      Interactions.RegisterClipped(Item.ClickAction, ButtonArea)
   end

   -- Draw the interface...
   NineGrid_Draw(NineGrid_GetHandle("itunesbox"), DisplayArea)

   local TriHandle
   if (Item.Dropped) then
      TriHandle = UIBitmap_GetHandle("itunesbox_arrow_pressed_dark")
   else
      TriHandle = UIBitmap_GetHandle("itunesbox_arrow_rest_dark")
   end
   UIBitmap_Draw(TriHandle, MakePoint(ButtonArea.x, ButtonArea.y))

   if (not Item.Editing) then
      if (Item.CurrEntry >= 1 and Item.CurrEntry <= #Item.Entries) then
         UIArea.ProtectedSimplePush(
            TextArea,
            function()
               RenderDropText(FontHandle, Entries[Item.CurrEntry], 0, MakePoint(0, FontY),
                              UIPref.GetColor("combo_text"),
                              UIPref.GetColor("combo_text_drop"))
            end)
      end
   else
      UIArea.ProtectedSimplePush(
         TextArea,
         function ()
            RenderTextWhileEditing(Item, MakePoint(0, Item.FontBaseline))
         end)
   end

   ITunesBoxDrawEntryList(Item)

   if (Item.IndexChanged) then
      local Changed = Item.IndexChanged
      Item.IndexChanged = false
      return Item.CurrEntry, Changed
   else
      return Item.CurrEntry, false
   end
end

local function Do(ID, Rect, Entries, CurrEntry)
   local RetVal,Changed = CurrEntry,false
   UIArea.ProtectedSimplePush(Rect, function()
                                       RetVal,Changed = DoITunesBased(ID, Entries, CurrEntry)
                                    end)
   return RetVal,Changed
end

local function DrawPlaceholder(Rect, String)
   local ButtonArea  = MakeRect(Rect.x + Rect.w - TriBitmapW, Rect.y,
                                TriBitmapW, TriBitmapH)
   local DisplayArea = MakeRect(Rect.x, Rect.y, Rect.w - TriBitmapW, Rect.h)
   local TextArea    = MakeRect(Rect.x + FontX, Rect.y, Rect.w - TriBitmapW - FontX, Rect.h)

   -- Draw the interface...
   NineGrid_Draw(NineGrid_GetHandle("combobox_nonedit"), DisplayArea)
   UIBitmap_Draw(UIBitmap_GetHandle("combobox_arrow_dark_disabled"),
                 MakePoint(ButtonArea.x, ButtonArea.y))

   if (String) then
      UIArea.ProtectedSimplePush(
         TextArea,
         function()
            RenderDropText(FontHandle, String, 0, MakePoint(0, FontY),
                           UIPref.GetColor("combo_text_placeholder"),
                           UIPref.GetColor("combo_text_drop"))
         end)
   end
end



ITunesBox = {
   DrawPlaceholder = DrawPlaceholder,
   Do = Do,
   RecommendHeight = function() return CBBitmapH end,
   FontOffset      = function() return FontY end,
}
