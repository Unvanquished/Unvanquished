# Create libRocket.

set(RMLUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/RmlUi)

set(Core_HDR_FILES
    ${RMLUI_DIR}/Source/Core/Clock.h
    ${RMLUI_DIR}/Source/Core/ComputeProperty.h
    ${RMLUI_DIR}/Source/Core/ContextInstancerDefault.h
    ${RMLUI_DIR}/Source/Core/DataController.h
    ${RMLUI_DIR}/Source/Core/DataControllerDefault.h
    ${RMLUI_DIR}/Source/Core/DataExpression.h
    ${RMLUI_DIR}/Source/Core/DataModel.h
    ${RMLUI_DIR}/Source/Core/DataView.h
    ${RMLUI_DIR}/Source/Core/DataViewDefault.h
    ${RMLUI_DIR}/Source/Core/DecoratorGradient.h
    ${RMLUI_DIR}/Source/Core/DecoratorNinePatch.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiled.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledBox.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledBoxInstancer.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledHorizontal.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledHorizontalInstancer.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledImage.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledImageInstancer.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledInstancer.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledVertical.h
    ${RMLUI_DIR}/Source/Core/DecoratorTiledVerticalInstancer.h
    ${RMLUI_DIR}/Source/Core/DocumentHeader.h
    ${RMLUI_DIR}/Source/Core/ElementAnimation.h
    ${RMLUI_DIR}/Source/Core/ElementBackgroundBorder.h
    ${RMLUI_DIR}/Source/Core/ElementDecoration.h
    ${RMLUI_DIR}/Source/Core/ElementDefinition.h
    ${RMLUI_DIR}/Source/Core/ElementHandle.h
    ${RMLUI_DIR}/Source/Core/Elements/ElementImage.h
    ${RMLUI_DIR}/Source/Core/Elements/ElementLabel.h
    ${RMLUI_DIR}/Source/Core/Elements/ElementTextSelection.h
    ${RMLUI_DIR}/Source/Core/Elements/InputType.h
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeButton.h
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeCheckbox.h
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeRadio.h
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeRange.h
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeSubmit.h
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeText.h
    ${RMLUI_DIR}/Source/Core/Elements/WidgetDropDown.h
    ${RMLUI_DIR}/Source/Core/Elements/WidgetSlider.h
    ${RMLUI_DIR}/Source/Core/Elements/WidgetTextInput.h
    ${RMLUI_DIR}/Source/Core/Elements/WidgetTextInputMultiLine.h
    ${RMLUI_DIR}/Source/Core/Elements/WidgetTextInputSingleLine.h
    ${RMLUI_DIR}/Source/Core/Elements/WidgetTextInputSingleLinePassword.h
    ${RMLUI_DIR}/Source/Core/Elements/XMLNodeHandlerDataGrid.h
    ${RMLUI_DIR}/Source/Core/Elements/XMLNodeHandlerSelect.h
    ${RMLUI_DIR}/Source/Core/Elements/XMLNodeHandlerTabSet.h
    ${RMLUI_DIR}/Source/Core/Elements/XMLNodeHandlerTextArea.h
    ${RMLUI_DIR}/Source/Core/ElementStyle.h
    ${RMLUI_DIR}/Source/Core/EventDispatcher.h
    ${RMLUI_DIR}/Source/Core/EventInstancerDefault.h
    ${RMLUI_DIR}/Source/Core/EventSpecification.h
    ${RMLUI_DIR}/Source/Core/FileInterfaceDefault.h
    ${RMLUI_DIR}/Source/Core/FontEffectBlur.h
    ${RMLUI_DIR}/Source/Core/FontEffectGlow.h
    ${RMLUI_DIR}/Source/Core/FontEffectOutline.h
    ${RMLUI_DIR}/Source/Core/FontEffectShadow.h
    ${RMLUI_DIR}/Source/Core/GeometryBackgroundBorder.h
    ${RMLUI_DIR}/Source/Core/GeometryDatabase.h
    ${RMLUI_DIR}/Source/Core/IdNameMap.h
    ${RMLUI_DIR}/Source/Core/LayoutBlockBox.h
    ${RMLUI_DIR}/Source/Core/LayoutBlockBoxSpace.h
    ${RMLUI_DIR}/Source/Core/LayoutDetails.h
    ${RMLUI_DIR}/Source/Core/LayoutEngine.h
    ${RMLUI_DIR}/Source/Core/LayoutFlex.h
    ${RMLUI_DIR}/Source/Core/LayoutInlineBox.h
    ${RMLUI_DIR}/Source/Core/LayoutInlineBoxText.h
    ${RMLUI_DIR}/Source/Core/LayoutLineBox.h
    ${RMLUI_DIR}/Source/Core/LayoutTable.h
    ${RMLUI_DIR}/Source/Core/LayoutTableDetails.h
    ${RMLUI_DIR}/Source/Core/Memory.h
    ${RMLUI_DIR}/Source/Core/PluginRegistry.h
    ${RMLUI_DIR}/Source/Core/Pool.h
    ${RMLUI_DIR}/Source/Core/precompiled.h
    ${RMLUI_DIR}/Source/Core/PropertiesIterator.h
    ${RMLUI_DIR}/Source/Core/PropertyParserAnimation.h
    ${RMLUI_DIR}/Source/Core/PropertyParserColour.h
    ${RMLUI_DIR}/Source/Core/PropertyParserDecorator.h
    ${RMLUI_DIR}/Source/Core/PropertyParserFontEffect.h
    ${RMLUI_DIR}/Source/Core/PropertyParserKeyword.h
    ${RMLUI_DIR}/Source/Core/PropertyParserNumber.h
    ${RMLUI_DIR}/Source/Core/PropertyParserRatio.h
    ${RMLUI_DIR}/Source/Core/PropertyParserString.h
    ${RMLUI_DIR}/Source/Core/PropertyParserTransform.h
    ${RMLUI_DIR}/Source/Core/PropertyShorthandDefinition.h
    ${RMLUI_DIR}/Source/Core/StreamFile.h
    ${RMLUI_DIR}/Source/Core/StyleSheetFactory.h
    ${RMLUI_DIR}/Source/Core/StyleSheetNode.h
    ${RMLUI_DIR}/Source/Core/StyleSheetParser.h
    ${RMLUI_DIR}/Source/Core/StyleSheetSelector.h
    ${RMLUI_DIR}/Source/Core/Template.h
    ${RMLUI_DIR}/Source/Core/TemplateCache.h
    ${RMLUI_DIR}/Source/Core/TextureDatabase.h
    ${RMLUI_DIR}/Source/Core/TextureLayout.h
    ${RMLUI_DIR}/Source/Core/TextureLayoutRectangle.h
    ${RMLUI_DIR}/Source/Core/TextureLayoutRow.h
    ${RMLUI_DIR}/Source/Core/TextureLayoutTexture.h
    ${RMLUI_DIR}/Source/Core/TextureResource.h
    ${RMLUI_DIR}/Source/Core/TransformState.h
    ${RMLUI_DIR}/Source/Core/TransformUtilities.h
    ${RMLUI_DIR}/Source/Core/WidgetScroll.h
    ${RMLUI_DIR}/Source/Core/XMLNodeHandlerBody.h
    ${RMLUI_DIR}/Source/Core/XMLNodeHandlerDefault.h
    ${RMLUI_DIR}/Source/Core/XMLNodeHandlerHead.h
    ${RMLUI_DIR}/Source/Core/XMLNodeHandlerTemplate.h
    ${RMLUI_DIR}/Source/Core/XMLParseTools.h
)

set(MASTER_Core_PUB_HDR_FILES
    ${RMLUI_DIR}/Include/RmlUi/Core.h
)

set(Core_PUB_HDR_FILES
    ${RMLUI_DIR}/Include/RmlUi/Config/Config.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Animation.h
    ${RMLUI_DIR}/Include/RmlUi/Core/BaseXMLParser.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Box.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Colour.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Colour.inl
    ${RMLUI_DIR}/Include/RmlUi/Core/ComputedValues.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Containers/chobo/flat_map.hpp
    ${RMLUI_DIR}/Include/RmlUi/Core/Containers/chobo/flat_set.hpp
    ${RMLUI_DIR}/Include/RmlUi/Core/Containers/robin_hood.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Context.h
    ${RMLUI_DIR}/Include/RmlUi/Core/ContextInstancer.h
    ${RMLUI_DIR}/Include/RmlUi/Core/ConvolutionFilter.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Core.h
    ${RMLUI_DIR}/Include/RmlUi/Core/DataModelHandle.h
    ${RMLUI_DIR}/Include/RmlUi/Core/DataStructHandle.h
    ${RMLUI_DIR}/Include/RmlUi/Core/DataTypeRegister.h
    ${RMLUI_DIR}/Include/RmlUi/Core/DataTypes.h
    ${RMLUI_DIR}/Include/RmlUi/Core/DataVariable.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Debug.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Decorator.h
    ${RMLUI_DIR}/Include/RmlUi/Core/DecoratorInstancer.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Dictionary.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Element.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Element.inl
    ${RMLUI_DIR}/Include/RmlUi/Core/ElementDocument.h
    ${RMLUI_DIR}/Include/RmlUi/Core/ElementInstancer.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/DataFormatter.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/DataQuery.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/DataSource.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/DataSourceListener.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementDataGrid.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementDataGridCell.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementDataGridExpandButton.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementDataGridRow.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementForm.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementFormControl.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementFormControlDataSelect.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementFormControlInput.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementFormControlSelect.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementFormControlTextArea.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementProgress.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Elements/ElementTabSet.h
    ${RMLUI_DIR}/Include/RmlUi/Core/ElementScroll.h
    ${RMLUI_DIR}/Include/RmlUi/Core/ElementText.h
    ${RMLUI_DIR}/Include/RmlUi/Core/ElementUtilities.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Event.h
    ${RMLUI_DIR}/Include/RmlUi/Core/EventInstancer.h
    ${RMLUI_DIR}/Include/RmlUi/Core/EventListener.h
    ${RMLUI_DIR}/Include/RmlUi/Core/EventListenerInstancer.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Factory.h
    ${RMLUI_DIR}/Include/RmlUi/Core/FileInterface.h
    ${RMLUI_DIR}/Include/RmlUi/Core/FontEffect.h
    ${RMLUI_DIR}/Include/RmlUi/Core/FontEffectInstancer.h
    ${RMLUI_DIR}/Include/RmlUi/Core/FontEngineInterface.h
    ${RMLUI_DIR}/Include/RmlUi/Core/FontGlyph.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Geometry.h
    ${RMLUI_DIR}/Include/RmlUi/Core/GeometryUtilities.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Header.h
    ${RMLUI_DIR}/Include/RmlUi/Core/ID.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Input.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Log.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Math.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Matrix4.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Matrix4.inl
    ${RMLUI_DIR}/Include/RmlUi/Core/ObserverPtr.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Platform.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Plugin.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Profiling.h
    ${RMLUI_DIR}/Include/RmlUi/Core/PropertiesIteratorView.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Property.h
    ${RMLUI_DIR}/Include/RmlUi/Core/PropertyDefinition.h
    ${RMLUI_DIR}/Include/RmlUi/Core/PropertyDictionary.h
    ${RMLUI_DIR}/Include/RmlUi/Core/PropertyIdSet.h
    ${RMLUI_DIR}/Include/RmlUi/Core/PropertyParser.h
    ${RMLUI_DIR}/Include/RmlUi/Core/PropertySpecification.h
    ${RMLUI_DIR}/Include/RmlUi/Core/RenderInterface.h
    ${RMLUI_DIR}/Include/RmlUi/Core/ScriptInterface.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Spritesheet.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Stream.h
    ${RMLUI_DIR}/Include/RmlUi/Core/StreamMemory.h
    ${RMLUI_DIR}/Include/RmlUi/Core/StringUtilities.h
    ${RMLUI_DIR}/Include/RmlUi/Core/StyleSheet.h
    ${RMLUI_DIR}/Include/RmlUi/Core/StyleSheetContainer.h
    ${RMLUI_DIR}/Include/RmlUi/Core/StyleSheetSpecification.h
    ${RMLUI_DIR}/Include/RmlUi/Core/StyleSheetTypes.h
    ${RMLUI_DIR}/Include/RmlUi/Core/StyleTypes.h
    ${RMLUI_DIR}/Include/RmlUi/Core/SystemInterface.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Texture.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Traits.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Transform.h
    ${RMLUI_DIR}/Include/RmlUi/Core/TransformPrimitive.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Tween.h
    ${RMLUI_DIR}/Include/RmlUi/Core/TypeConverter.h
    ${RMLUI_DIR}/Include/RmlUi/Core/TypeConverter.inl
    ${RMLUI_DIR}/Include/RmlUi/Core/Types.h
    ${RMLUI_DIR}/Include/RmlUi/Core/URL.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Utilities.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Variant.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Variant.inl
    ${RMLUI_DIR}/Include/RmlUi/Core/Vector2.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Vector2.inl
    ${RMLUI_DIR}/Include/RmlUi/Core/Vector3.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Vector3.inl
    ${RMLUI_DIR}/Include/RmlUi/Core/Vector4.h
    ${RMLUI_DIR}/Include/RmlUi/Core/Vector4.inl
    ${RMLUI_DIR}/Include/RmlUi/Core/Vertex.h
    ${RMLUI_DIR}/Include/RmlUi/Core/XMLNodeHandler.h
    ${RMLUI_DIR}/Include/RmlUi/Core/XMLParser.h
)

set(Core_SRC_FILES
    ${RMLUI_DIR}/Source/Core/BaseXMLParser.cpp
    ${RMLUI_DIR}/Source/Core/Box.cpp
    ${RMLUI_DIR}/Source/Core/Clock.cpp
    ${RMLUI_DIR}/Source/Core/ComputedValues.cpp
    ${RMLUI_DIR}/Source/Core/ComputeProperty.cpp
    ${RMLUI_DIR}/Source/Core/Context.cpp
    ${RMLUI_DIR}/Source/Core/ContextInstancer.cpp
    ${RMLUI_DIR}/Source/Core/ContextInstancerDefault.cpp
    ${RMLUI_DIR}/Source/Core/ConvolutionFilter.cpp
    ${RMLUI_DIR}/Source/Core/Core.cpp
    ${RMLUI_DIR}/Source/Core/DataController.cpp
    ${RMLUI_DIR}/Source/Core/DataControllerDefault.cpp
    ${RMLUI_DIR}/Source/Core/DataExpression.cpp
    ${RMLUI_DIR}/Source/Core/DataModel.cpp
    ${RMLUI_DIR}/Source/Core/DataModelHandle.cpp
    ${RMLUI_DIR}/Source/Core/DataTypeRegister.cpp
    ${RMLUI_DIR}/Source/Core/DataVariable.cpp
    ${RMLUI_DIR}/Source/Core/DataView.cpp
    ${RMLUI_DIR}/Source/Core/DataViewDefault.cpp
    ${RMLUI_DIR}/Source/Core/Decorator.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorGradient.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorInstancer.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorNinePatch.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiled.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledBox.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledBoxInstancer.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledHorizontal.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledHorizontalInstancer.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledImage.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledImageInstancer.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledInstancer.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledVertical.cpp
    ${RMLUI_DIR}/Source/Core/DecoratorTiledVerticalInstancer.cpp
    ${RMLUI_DIR}/Source/Core/DocumentHeader.cpp
    ${RMLUI_DIR}/Source/Core/Element.cpp
    ${RMLUI_DIR}/Source/Core/ElementAnimation.cpp
    ${RMLUI_DIR}/Source/Core/ElementBackgroundBorder.cpp
    ${RMLUI_DIR}/Source/Core/ElementDecoration.cpp
    ${RMLUI_DIR}/Source/Core/ElementDefinition.cpp
    ${RMLUI_DIR}/Source/Core/ElementDocument.cpp
    ${RMLUI_DIR}/Source/Core/ElementHandle.cpp
    ${RMLUI_DIR}/Source/Core/ElementInstancer.cpp
    ${RMLUI_DIR}/Source/Core/Elements/DataFormatter.cpp
    ${RMLUI_DIR}/Source/Core/Elements/DataQuery.cpp
    ${RMLUI_DIR}/Source/Core/Elements/DataSource.cpp
    ${RMLUI_DIR}/Source/Core/Elements/DataSourceListener.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementDataGrid.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementDataGridCell.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementDataGridExpandButton.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementDataGridRow.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementForm.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementFormControl.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementFormControlDataSelect.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementFormControlInput.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementFormControlSelect.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementFormControlTextArea.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementImage.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementLabel.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementProgress.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementTabSet.cpp
    ${RMLUI_DIR}/Source/Core/Elements/ElementTextSelection.cpp
    ${RMLUI_DIR}/Source/Core/Elements/InputType.cpp
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeButton.cpp
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeCheckbox.cpp
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeRadio.cpp
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeRange.cpp
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeSubmit.cpp
    ${RMLUI_DIR}/Source/Core/Elements/InputTypeText.cpp
    ${RMLUI_DIR}/Source/Core/Elements/WidgetDropDown.cpp
    ${RMLUI_DIR}/Source/Core/Elements/WidgetSlider.cpp
    ${RMLUI_DIR}/Source/Core/Elements/WidgetTextInput.cpp
    ${RMLUI_DIR}/Source/Core/Elements/WidgetTextInputMultiLine.cpp
    ${RMLUI_DIR}/Source/Core/Elements/WidgetTextInputSingleLine.cpp
    ${RMLUI_DIR}/Source/Core/Elements/WidgetTextInputSingleLinePassword.cpp
    ${RMLUI_DIR}/Source/Core/Elements/XMLNodeHandlerDataGrid.cpp
    ${RMLUI_DIR}/Source/Core/Elements/XMLNodeHandlerSelect.cpp
    ${RMLUI_DIR}/Source/Core/Elements/XMLNodeHandlerTabSet.cpp
    ${RMLUI_DIR}/Source/Core/Elements/XMLNodeHandlerTextArea.cpp
    ${RMLUI_DIR}/Source/Core/ElementScroll.cpp
    ${RMLUI_DIR}/Source/Core/ElementStyle.cpp
    ${RMLUI_DIR}/Source/Core/ElementText.cpp
    ${RMLUI_DIR}/Source/Core/ElementUtilities.cpp
    ${RMLUI_DIR}/Source/Core/Event.cpp
    ${RMLUI_DIR}/Source/Core/EventDispatcher.cpp
    ${RMLUI_DIR}/Source/Core/EventInstancer.cpp
    ${RMLUI_DIR}/Source/Core/EventInstancerDefault.cpp
    ${RMLUI_DIR}/Source/Core/EventListenerInstancer.cpp
    ${RMLUI_DIR}/Source/Core/EventSpecification.cpp
    ${RMLUI_DIR}/Source/Core/Factory.cpp
    ${RMLUI_DIR}/Source/Core/FileInterface.cpp
    ${RMLUI_DIR}/Source/Core/FileInterfaceDefault.cpp
    ${RMLUI_DIR}/Source/Core/FontEffect.cpp
    ${RMLUI_DIR}/Source/Core/FontEffectBlur.cpp
    ${RMLUI_DIR}/Source/Core/FontEffectGlow.cpp
    ${RMLUI_DIR}/Source/Core/FontEffectInstancer.cpp
    ${RMLUI_DIR}/Source/Core/FontEffectOutline.cpp
    ${RMLUI_DIR}/Source/Core/FontEffectShadow.cpp
    ${RMLUI_DIR}/Source/Core/FontEngineInterface.cpp
    ${RMLUI_DIR}/Source/Core/Geometry.cpp
    ${RMLUI_DIR}/Source/Core/GeometryBackgroundBorder.cpp
    ${RMLUI_DIR}/Source/Core/GeometryDatabase.cpp
    ${RMLUI_DIR}/Source/Core/GeometryUtilities.cpp
    ${RMLUI_DIR}/Source/Core/LayoutBlockBox.cpp
    ${RMLUI_DIR}/Source/Core/LayoutBlockBoxSpace.cpp
    ${RMLUI_DIR}/Source/Core/LayoutDetails.cpp
    ${RMLUI_DIR}/Source/Core/LayoutEngine.cpp
    ${RMLUI_DIR}/Source/Core/LayoutFlex.cpp
    ${RMLUI_DIR}/Source/Core/LayoutInlineBox.cpp
    ${RMLUI_DIR}/Source/Core/LayoutInlineBoxText.cpp
    ${RMLUI_DIR}/Source/Core/LayoutLineBox.cpp
    ${RMLUI_DIR}/Source/Core/LayoutTable.cpp
    ${RMLUI_DIR}/Source/Core/LayoutTableDetails.cpp
    ${RMLUI_DIR}/Source/Core/Log.cpp
    ${RMLUI_DIR}/Source/Core/Math.cpp
    ${RMLUI_DIR}/Source/Core/Memory.cpp
    ${RMLUI_DIR}/Source/Core/ObserverPtr.cpp
    ${RMLUI_DIR}/Source/Core/Plugin.cpp
    ${RMLUI_DIR}/Source/Core/PluginRegistry.cpp
    ${RMLUI_DIR}/Source/Core/Profiling.cpp
    ${RMLUI_DIR}/Source/Core/PropertiesIteratorView.cpp
    ${RMLUI_DIR}/Source/Core/Property.cpp
    ${RMLUI_DIR}/Source/Core/PropertyDefinition.cpp
    ${RMLUI_DIR}/Source/Core/PropertyDictionary.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserAnimation.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserColour.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserDecorator.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserFontEffect.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserKeyword.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserNumber.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserRatio.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserString.cpp
    ${RMLUI_DIR}/Source/Core/PropertyParserTransform.cpp
    ${RMLUI_DIR}/Source/Core/PropertySpecification.cpp
    ${RMLUI_DIR}/Source/Core/RenderInterface.cpp
    ${RMLUI_DIR}/Source/Core/Spritesheet.cpp
    ${RMLUI_DIR}/Source/Core/Stream.cpp
    ${RMLUI_DIR}/Source/Core/StreamFile.cpp
    ${RMLUI_DIR}/Source/Core/StreamMemory.cpp
    ${RMLUI_DIR}/Source/Core/StringUtilities.cpp
    ${RMLUI_DIR}/Source/Core/StyleSheet.cpp
    ${RMLUI_DIR}/Source/Core/StyleSheetContainer.cpp
    ${RMLUI_DIR}/Source/Core/StyleSheetFactory.cpp
    ${RMLUI_DIR}/Source/Core/StyleSheetNode.cpp
    ${RMLUI_DIR}/Source/Core/StyleSheetParser.cpp
    ${RMLUI_DIR}/Source/Core/StyleSheetSelector.cpp
    ${RMLUI_DIR}/Source/Core/StyleSheetSpecification.cpp
    ${RMLUI_DIR}/Source/Core/SystemInterface.cpp
    ${RMLUI_DIR}/Source/Core/Template.cpp
    ${RMLUI_DIR}/Source/Core/TemplateCache.cpp
    ${RMLUI_DIR}/Source/Core/Texture.cpp
    ${RMLUI_DIR}/Source/Core/TextureDatabase.cpp
    ${RMLUI_DIR}/Source/Core/TextureLayout.cpp
    ${RMLUI_DIR}/Source/Core/TextureLayoutRectangle.cpp
    ${RMLUI_DIR}/Source/Core/TextureLayoutRow.cpp
    ${RMLUI_DIR}/Source/Core/TextureLayoutTexture.cpp
    ${RMLUI_DIR}/Source/Core/TextureResource.cpp
    ${RMLUI_DIR}/Source/Core/Transform.cpp
    ${RMLUI_DIR}/Source/Core/TransformPrimitive.cpp
    ${RMLUI_DIR}/Source/Core/TransformState.cpp
    ${RMLUI_DIR}/Source/Core/TransformUtilities.cpp
    ${RMLUI_DIR}/Source/Core/Tween.cpp
    ${RMLUI_DIR}/Source/Core/TypeConverter.cpp
    ${RMLUI_DIR}/Source/Core/URL.cpp
    ${RMLUI_DIR}/Source/Core/Variant.cpp
    ${RMLUI_DIR}/Source/Core/WidgetScroll.cpp
    ${RMLUI_DIR}/Source/Core/XMLNodeHandler.cpp
    ${RMLUI_DIR}/Source/Core/XMLNodeHandlerBody.cpp
    ${RMLUI_DIR}/Source/Core/XMLNodeHandlerDefault.cpp
    ${RMLUI_DIR}/Source/Core/XMLNodeHandlerHead.cpp
    ${RMLUI_DIR}/Source/Core/XMLNodeHandlerTemplate.cpp
    ${RMLUI_DIR}/Source/Core/XMLParser.cpp
    ${RMLUI_DIR}/Source/Core/XMLParseTools.cpp
)

set(Debugger_HDR_FILES
    ${RMLUI_DIR}/Source/Debugger/BeaconSource.h
    ${RMLUI_DIR}/Source/Debugger/CommonSource.h
    ${RMLUI_DIR}/Source/Debugger/DebuggerPlugin.h
    ${RMLUI_DIR}/Source/Debugger/DebuggerSystemInterface.h
    ${RMLUI_DIR}/Source/Debugger/ElementContextHook.h
    ${RMLUI_DIR}/Source/Debugger/ElementInfo.h
    ${RMLUI_DIR}/Source/Debugger/ElementLog.h
    ${RMLUI_DIR}/Source/Debugger/FontSource.h
    ${RMLUI_DIR}/Source/Debugger/Geometry.h
    ${RMLUI_DIR}/Source/Debugger/InfoSource.h
    ${RMLUI_DIR}/Source/Debugger/LogSource.h
    ${RMLUI_DIR}/Source/Debugger/MenuSource.h
)

set(MASTER_Debugger_PUB_HDR_FILES
    ${RMLUI_DIR}/Include/RmlUi/Debugger.h
)

set(Debugger_PUB_HDR_FILES
    ${RMLUI_DIR}/Include/RmlUi/Debugger/Debugger.h
    ${RMLUI_DIR}/Include/RmlUi/Debugger/Header.h
)

set(Debugger_SRC_FILES
    ${RMLUI_DIR}/Source/Debugger/Debugger.cpp
    ${RMLUI_DIR}/Source/Debugger/DebuggerPlugin.cpp
    ${RMLUI_DIR}/Source/Debugger/DebuggerSystemInterface.cpp
    ${RMLUI_DIR}/Source/Debugger/ElementContextHook.cpp
    ${RMLUI_DIR}/Source/Debugger/ElementInfo.cpp
    ${RMLUI_DIR}/Source/Debugger/ElementLog.cpp
    ${RMLUI_DIR}/Source/Debugger/Geometry.cpp
)

set(Core_HDR_FILES
    ${Core_HDR_FILES}
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontEngineInterfaceDefault.h
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontFace.h
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontFaceHandleDefault.h
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontFaceLayer.h
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontFamily.h
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontProvider.h
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontTypes.h
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FreeTypeInterface.h
)

set(Core_SRC_FILES
    ${Core_SRC_FILES}
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontEngineInterfaceDefault.cpp
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontFace.cpp
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontFaceHandleDefault.cpp
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontFaceLayer.cpp
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontFamily.cpp
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FontProvider.cpp
    ${RMLUI_DIR}/Source/Core/FontEngineDefault/FreeTypeInterface.cpp
)

set(Lua_HDR_FILES
    ${RMLUI_DIR}/Source/Lua/Colourb.h
    ${RMLUI_DIR}/Source/Lua/Colourf.h
    ${RMLUI_DIR}/Source/Lua/Context.h
    ${RMLUI_DIR}/Source/Lua/ContextDocumentsProxy.h
    ${RMLUI_DIR}/Source/Lua/Document.h
    ${RMLUI_DIR}/Source/Lua/Element.h
    ${RMLUI_DIR}/Source/Lua/ElementAttributesProxy.h
    ${RMLUI_DIR}/Source/Lua/ElementChildNodesProxy.h
    ${RMLUI_DIR}/Source/Lua/ElementInstancer.h
    ${RMLUI_DIR}/Source/Lua/Elements/As.h
    ${RMLUI_DIR}/Source/Lua/Elements/DataFormatter.h
    ${RMLUI_DIR}/Source/Lua/Elements/DataSource.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementDataGrid.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementDataGridRow.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementForm.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControl.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControlDataSelect.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControlInput.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControlSelect.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControlTextArea.h
    ${RMLUI_DIR}/Source/Lua/Elements/ElementTabSet.h
    ${RMLUI_DIR}/Source/Lua/Elements/LuaDataFormatter.h
    ${RMLUI_DIR}/Source/Lua/Elements/LuaDataSource.h
    ${RMLUI_DIR}/Source/Lua/Elements/SelectOptionsProxy.h
    ${RMLUI_DIR}/Source/Lua/ElementStyleProxy.h
    ${RMLUI_DIR}/Source/Lua/ElementText.h
    ${RMLUI_DIR}/Source/Lua/Event.h
    ${RMLUI_DIR}/Source/Lua/EventParametersProxy.h
    ${RMLUI_DIR}/Source/Lua/GlobalLuaFunctions.h
    ${RMLUI_DIR}/Source/Lua/Log.h
    ${RMLUI_DIR}/Source/Lua/LuaDataModel.h
    ${RMLUI_DIR}/Source/Lua/LuaDocument.h
    ${RMLUI_DIR}/Source/Lua/LuaDocumentElementInstancer.h
    ${RMLUI_DIR}/Source/Lua/LuaElementInstancer.h
    ${RMLUI_DIR}/Source/Lua/LuaEventListener.h
    ${RMLUI_DIR}/Source/Lua/LuaEventListenerInstancer.h
    ${RMLUI_DIR}/Source/Lua/LuaPlugin.h
    ${RMLUI_DIR}/Source/Lua/Pairs.h
    ${RMLUI_DIR}/Source/Lua/RmlUi.h
    ${RMLUI_DIR}/Source/Lua/RmlUiContextsProxy.h
    ${RMLUI_DIR}/Source/Lua/Vector2f.h
    ${RMLUI_DIR}/Source/Lua/Vector2i.h
)

set(Lua_PUB_HDR_FILES
    ${RMLUI_DIR}/Include/RmlUi/Lua/Header.h
    ${RMLUI_DIR}/Include/RmlUi/Lua/IncludeLua.h
    ${RMLUI_DIR}/Include/RmlUi/Lua/Interpreter.h
    ${RMLUI_DIR}/Include/RmlUi/Lua/Lua.h
    ${RMLUI_DIR}/Include/RmlUi/Lua/LuaType.h
    ${RMLUI_DIR}/Include/RmlUi/Lua/Utilities.h
)

set(Lua_SRC_FILES
    ${RMLUI_DIR}/Source/Lua/Colourb.cpp
    ${RMLUI_DIR}/Source/Lua/Colourf.cpp
    ${RMLUI_DIR}/Source/Lua/Context.cpp
    ${RMLUI_DIR}/Source/Lua/ContextDocumentsProxy.cpp
    ${RMLUI_DIR}/Source/Lua/Document.cpp
    ${RMLUI_DIR}/Source/Lua/Element.cpp
    ${RMLUI_DIR}/Source/Lua/ElementAttributesProxy.cpp
    ${RMLUI_DIR}/Source/Lua/ElementChildNodesProxy.cpp
    ${RMLUI_DIR}/Source/Lua/ElementInstancer.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/DataFormatter.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/DataSource.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementDataGrid.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementDataGridRow.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementForm.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControl.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControlDataSelect.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControlInput.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControlSelect.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementFormControlTextArea.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/ElementTabSet.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/LuaDataFormatter.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/LuaDataSource.cpp
    ${RMLUI_DIR}/Source/Lua/Elements/SelectOptionsProxy.cpp
    ${RMLUI_DIR}/Source/Lua/ElementStyleProxy.cpp
    ${RMLUI_DIR}/Source/Lua/ElementText.cpp
    ${RMLUI_DIR}/Source/Lua/Event.cpp
    ${RMLUI_DIR}/Source/Lua/EventParametersProxy.cpp
    ${RMLUI_DIR}/Source/Lua/GlobalLuaFunctions.cpp
    ${RMLUI_DIR}/Source/Lua/Interpreter.cpp
    ${RMLUI_DIR}/Source/Lua/Log.cpp
    ${RMLUI_DIR}/Source/Lua/Lua.cpp
    ${RMLUI_DIR}/Source/Lua/LuaDataModel.cpp
    ${RMLUI_DIR}/Source/Lua/LuaDocument.cpp
    ${RMLUI_DIR}/Source/Lua/LuaDocumentElementInstancer.cpp
    ${RMLUI_DIR}/Source/Lua/LuaElementInstancer.cpp
    ${RMLUI_DIR}/Source/Lua/LuaEventListener.cpp
    ${RMLUI_DIR}/Source/Lua/LuaEventListenerInstancer.cpp
    ${RMLUI_DIR}/Source/Lua/LuaPlugin.cpp
    ${RMLUI_DIR}/Source/Lua/LuaType.cpp
    ${RMLUI_DIR}/Source/Lua/RmlUi.cpp
    ${RMLUI_DIR}/Source/Lua/RmlUiContextsProxy.cpp
    ${RMLUI_DIR}/Source/Lua/Utilities.cpp
    ${RMLUI_DIR}/Source/Lua/Vector2f.cpp
    ${RMLUI_DIR}/Source/Lua/Vector2i.cpp
)

if (NOT FREETYPE_INCLUDE_DIRS)
    find_package(Freetype REQUIRED)
endif()
include_directories(${FREETYPE_INCLUDE_DIRS})

set(RMLUI_INCLUDE_DIRS ${RMLUI_DIR}/Include)
include_directories(${RMLUI_INCLUDE_DIRS})
add_library(RMLUI_LIB STATIC
    ${Core_HDR_FILES}
    ${MASTER_Core_PUB_HDR_FILES}
    ${Core_PUB_HDR_FILES}
    ${Core_SRC_FILES}
    ${Debugger_HDR_FILES}
    ${MASTER_Debugger_PUB_HDR_FILES}
    ${Debugger_PUB_HDR_FILES}
    ${Debugger_SRC_FILES}
    ${Lua_HDR_FILES}
    ${Lua_PUB_HDR_FILES}
    ${Lua_SRC_FILES}
)

set_property(TARGET RMLUI_LIB PROPERTY
    COMPILE_DEFINITIONS RMLUI_STATIC_LIB LUA_COMPAT_APIINTCASTS
)
set_target_properties(RMLUI_LIB PROPERTIES
    POSITION_INDEPENDENT_CODE 1
)
