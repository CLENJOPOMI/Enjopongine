﻿#include "GameObject.h"
#include "BaseComponent.h"
#include "Globals.h"
#include <GL/glew.h>
#include "TransformComponent.h"
#include <MathGeoLib/include/Math/float4x4.h>
#include "Engine.h"
#include "ModuleEditor.h"

GameObject::GameObject()
{
	BoundingBox.SetNegativeInfinity();
}

GameObject::~GameObject()
{
}

void GameObject::SetParent(GameObject* new_parent)
{
	if(_parent != nullptr)
	{
		_parent->RemoveChild(this);
	}
	new_parent->_childs.push_back(this);
	_parent = new_parent;
}

GameObject* GameObject::GetParent() const
{
	return _parent;
}

const std::vector<GameObject*>& GameObject::GetChilds() const
{
	return _childs;
}

void GameObject::AddChild(GameObject* child)
{
	if (child != nullptr)
	{
		child->SetParent(this);
	}
}

void GameObject::RemoveChild(const GameObject* child)
{
	if(!_childs.empty())
	{
		for (auto it = _childs.begin(); it != _childs.cend(); ++it)
		{
			if (*it == child)
			{
				_childs.erase(it);
				break;
			}
		}
	}
}

const std::list<BaseComponent*>& GameObject::GetComponents() const
{
	return _components;
}

void GameObject::AddComponent(BaseComponent* component)
{
	if (component != nullptr)
	{
		component->Parent = this;
		_components.push_back(component);

		if (component->Name == "Transform")
			_transform = static_cast<TransformComponent*>(component);
	}
}

BaseComponent* GameObject::GetComponentByName(const std::string& name) const
{
	for (BaseComponent* component : _components)
	{
		if (component->Name == name)
			return component;
	}

	return nullptr;
}

void GameObject::DeleteComponentByName(const std::string& name)
{
	if(!_components.empty())
	{
		for (auto it = _components.begin(); it != _components.cend(); ++it)
		{
			if ((*it)->Name == name)
			{
				_components.erase(it);
				(*it)->CleanUp();
				RELEASE(*it);
			}
		}
	}
}

void GameObject::DeleteComponent(BaseComponent* component)
{
	_components.remove(component);
}

TransformComponent* GameObject::GetTransform() const
{
	return _transform;
}

void GameObject::DrawBoundingBox()
{
	::DrawBoundingBox(BoundingBox);
}

void GameObject::DrawHierachy()
{
	GLboolean light = glIsEnabled(GL_LIGHTING);
	glDisable(GL_LIGHTING);
	glColor4f(0.f, 0.f, 1.f, 1.f);

	float4x4 transform = float4x4::identity;
	for (GameObject* child : _childs)
		child->DrawHierachy(transform);

	glColor4f(1.f, 1.f, 1.f, 1.f);

	if (light)
		glEnable(GL_LIGHTING);
}

void GameObject::DrawHierachy(const float4x4& transformMatrix)
{
	float4x4 localMatrix = transformMatrix * _transform->GetTransformMatrix();

	if (_parent && _parent->_transform)
	{
		glBegin(GL_LINES);
		float3 parentPos = transformMatrix.Col3(3);
		glVertex3fv(reinterpret_cast<GLfloat*>(&parentPos));
		float3 position = localMatrix.Col3(3);
		glVertex3fv(reinterpret_cast<GLfloat*>(&position));
		glEnd();
	}

	for (GameObject* child : _childs)
		child->DrawHierachy(localMatrix);
}


void GameObject::Update(float dt)
{
	glPushMatrix();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	for (BaseComponent* baseComponent : _components)
	{
		if (baseComponent->Enabled)
		{
			if (App->editor->IsPlaying())
			{
				if (_isPlaying)
					baseComponent->Update(dt);
				else // TODO: When serialization is available, back up gameobject tree to disk
				{
					BaseComponent::CreateBackup(baseComponent);
					baseComponent->BeginPlay();
				}
			}
			else
			{
				if (_isPlaying) // TODO: When serialization is available, restore gameobject tree from disk
				{
					baseComponent->EndPlay();
					BaseComponent::RestoreBackup(baseComponent);
				}
				else
					baseComponent->EditorUpdate(App->editor->IsPaused() ? 0 : dt);
			}
		
		
		}
	}

	_isPlaying = App->editor->IsPlaying();

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	for (GameObject* child : _childs)
	{
		child->Update(dt);
	}

	glPopMatrix();
}

bool GameObject::CleanUp()
{
	for (BaseComponent* component : _components)
	{
		component->CleanUp();
		RELEASE(component);
	}
	return true;
}

