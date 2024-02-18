function EvntStr(event)
	if event == nil then
		return "nil"
	end
	return string.format("%s[%s] from %s, current: %s", tostring(event), event.type, event.target_element.tag_name, event.current_element.tag_name)
end

CircleMenu = {
	-- Default values
	menuRadiusEm = 10,	-- radius on which buttons are placed
	hintsRadiusEm = 6.3,	-- radius on which hints are placed
}

CircleMenu.__index = CircleMenu

function CircleMenu:New(handle, idPrefix) 
	local instance = { rt = {} }
	setmetatable(instance, self)
	instance.idPrefix = idPrefix
	instance.handle = handle

	return instance
end

function CircleMenu:Bind(document)
	self.document = document
	self.menuElement = document:GetElementById(self.idPrefix .. 'Menu')
	self.keyHintsElement = document:GetElementById(self.idPrefix .. 'KeyboardHints')
	self.backdropElement = document:GetElementById(self.idPrefix .. 'Backdrop')
end

function CircleMenu:BuildMenu()
	local menu_rml, hints_rml
	menu_rml= string.format(
		'<button class="cancelButton" onmouseover="%s:Highlight(0)" onclick="%s:Quit(event)"><span>X</span></button>', 
		self.handle, self.handle)
	hints_rml = ''
	local tau = 2 * math.pi / #self.items
	for i=1,#self.items do
		local x, y = NormalCartesian(tau * (i - 1))
		menu_rml = menu_rml .. string.format(
			'<div style="position: absolute; left: %.1fem; top: %.1fem;">%s</div>',
			x * self.menuRadiusEm, y * self.menuRadiusEm, self:RMLButton(i)
		)
		hints_rml = hints_rml .. string.format(
			'<div style="position: absolute; left: %.1fem; top: %.1fem;"><p class="key">%d</p></div>',
			x * self.hintsRadiusEm, y * self.hintsRadiusEm, i % 10
		)
	end
	self.menuElement.inner_rml = menu_rml
	self.keyHintsElement.inner_rml = hints_rml
end

function CircleMenu:Build(document)
	self:Bind(document)
	self:PopulateItems()
	self:Highlight(nil) -- clear info
	self:BuildMenu()
	self.quitting = false
end


-- EVENT HANDLERS
function CircleMenu:Hover(index)
	self:Highlight(index)
end

function CircleMenu:Click(index, event)
	self:Select(index, event)
end

function CircleMenu:BackdropCapture(event)
	local index = CirclemenuMouseToIndex(self.document, event, #self.items)
	-- Cancel button is first
	self:Highlight(index)
	self.menuElement.child_nodes[index+1].first_child:Focus()
end

function CircleMenu:BackdropClick(event)
	local index = CirclemenuMouseToIndex(self.document, event, #self.items)
	self:Select(index, event)
end

function CircleMenu:HandleKey(event)
	local key = event.parameters["key_identifier"]
	local index
	if key == rmlui.key_identifier["0"] then
		index = 10
	elseif key >= rmlui.key_identifier["1"] and key <= rmlui.key_identifier["9"] then
		index = key - rmlui.key_identifier["1"] + 1
	else
		detectEscape(event, self.document)
		return
	end
	if index <= #self.items then
		event:StopPropagation()
		self.menuElement.child_nodes[index+1].first_child:Click()
	end
end

function CircleMenu:Quit(event)
	-- Hacky workaround to event:StopPropagation() not stopping the propagation half the time
	-- NOTE: Hiding already hidden menus may cause crashes!
	if self.document == nil then
		-- Nothing to do
		return
	end
	if self.quitting then
		-- Already quiting
		return
	end
	self.quitting = true

	if event == nil then
		-- Fallback option
		self.document:Hide()
	else
		Events.pushevent(string.format("hide %s", self.document.attributes.id), event)
	end
end


-- PLACEHOLDER METHODS

function CircleMenu:PopulateItems()
	-- Placeholder method for obtaining data to create menu items
end

function CircleMenu:RMLButton(index)
	-- Placeholder method for buidling RML of a button
end

function CircleMenu:Select(index, event)
	-- Placeholder method for picking an option
end

function CircleMenu:Highlight(index)
	-- Placeholder method for highlighting a option
end
