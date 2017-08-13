#pragma push_macro("new")
#undef new
#include <BulletDynamics/Dynamics/btRigidBody.h>
#pragma pop_macro("new")
#include "RigidBodyComponent.h"
#include "GameObject.h"
#include "Engine.h"
#include "ModulePhysics.h"
#include "TransformComponent.h"
#include <MathGeoLib/include/Math/float4x4.h>
#include <MathGeoLib/include/Math/float3x3.h>
#include "IMGUI/imgui.h"

RigidBodyComponent::RigidBodyComponent()
{
	Name = "BoxCollider";
}

RigidBodyComponent::~RigidBodyComponent()
{
}

void RigidBodyComponent::Attached()
{
	AABB boundingBox = Parent->BoundingBox;
	vec size = boundingBox.HalfSize();
	_gravity = App->physics->GetGravity();
	_center = boundingBox.CenterPoint();
	SetSize(size);
}

void RigidBodyComponent::BeginPlay()
{
	createBody();
}

void RigidBodyComponent::Update(float dt)
{
}

void RigidBodyComponent::EndPlay()
{
	App->physics->RemoveBody(_rigidBody);
	_rigidBody = nullptr;
}

void RigidBodyComponent::CleanUp()
{
	BaseComponent::CleanUp();

	if (_rigidBody != nullptr)
		App->physics->RemoveBody(_rigidBody);
	_rigidBody = nullptr;
}

void RigidBodyComponent::DrawUI()
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlapMode;
	if (ImGui::TreeNodeEx("Collider shape", flags))
	{
		int colliderShape = _shape;
		ImGui::Combo("Collider shape", &colliderShape, "Box\0Sphere\0Capsule\0Cylinder\0");
		_shape = static_cast<ColliderShape>(colliderShape);

		switch (_shape)
		{
		case BOX:
		case CYLINDER:
			ImGui::InputFloat3("Size", &_colliderConfig[0], -1, ImGuiInputTextFlags_CharsDecimal);
			break;
		case SPHERE:
			ImGui::InputFloat("Radius", &_colliderConfig[0], -1, ImGuiInputTextFlags_CharsDecimal);
			break;
		case CAPSULE:
			ImGui::InputFloat2("Extents", &_colliderConfig[0], -1, ImGuiInputTextFlags_CharsDecimal);
			break;
		}
		ImGui::InputFloat3("Center", &_center[0], -1, ImGuiInputTextFlags_CharsDecimal);

		if (_rigidBody && ImGui::Button("Commit to physics engine"))
			createBody();

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Physic configuration", flags))
	{
		bool kinematic = _isKinematic;
		if (ImGui::Checkbox("Kinematic", &kinematic))
		{
			SetKinematic(kinematic);
		}

		if (ImGui::DragFloat3("Linear factor", &_linearFactor[0], 0.05f, 0.f, 1.f))
		{
			SetLinearFactor(_linearFactor);
		}

		if (ImGui::DragFloat3("Angular factor", &_angularFactor[0], 0.05f, 0.f, 1.f))
		{
			SetAngularFactor(_angularFactor);
		}

		if (ImGui::InputFloat3("Gravity", &_gravity[0], -1, ImGuiInputTextFlags_CharsDecimal))
		{
			SetGravity(_gravity);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Physic properties", flags))
	{
		if (ImGui::DragFloat("Mass", &_mass, 0.1f, 0.1f, 100000.f))
		{
			SetMass(_mass);
		}

		if (ImGui::DragFloat("Restitution", &_restitution, 0.1f))
		{
			SetRestitution(_restitution);
		}

		ImGui::TreePop();
	}
}

void RigidBodyComponent::getWorldTransform(btTransform& worldTrans) const
{
	float4x4 transform = Parent->GetTransform()->GetTransformMatrix();
	float3 translate = transform.TranslatePart() + _center;
	Quat rot = transform.RotatePart().ToQuat();
	worldTrans.setOrigin(btVector3(translate.x, translate.y, translate.z));
	worldTrans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
}

void RigidBodyComponent::setWorldTransform(const btTransform& worldTrans)
{
	btQuaternion rot = worldTrans.getRotation();
	btVector3 pos = worldTrans.getOrigin();

	Quat rotation = Quat(rot.x(), rot.y(), rot.z(), rot.w());
	float4x4 new_global(rotation, float3(pos.x(), pos.y(), pos.z()) - rotation.Mul(_center));
	Parent->GetTransform()->SetTransformMatrix(new_global);
}

void RigidBodyComponent::createBody()
{
	if (_rigidBody)
		App->physics->RemoveBody(_rigidBody);

	switch (_shape)
	{
	case BOX:
		_rigidBody = App->physics->AddBoxBody(_colliderConfig, this);
		break;
	case SPHERE:
		_rigidBody = App->physics->AddSphereBody(_colliderConfig.x, this);
		break;
	case CAPSULE:
		_rigidBody = App->physics->AddCapsuleBody(_colliderConfig.x, _colliderConfig.y, this);
		break;
	case CYLINDER:
		_rigidBody = App->physics->AddCylinderBody(_colliderConfig, this);
		break;
	}
	
	_rigidBody->setGravity(btVector3(_gravity.x, _gravity.y, _gravity.z));

	SetKinematic(_isKinematic);
	SetLinearFactor(_linearFactor);
	SetAngularFactor(_angularFactor);
	SetMass(_mass);
	SetRestitution(_restitution);
}

float RigidBodyComponent::GetMass() const
{
	return _mass;
}

void RigidBodyComponent::SetMass(float mass)
{
	_mass = mass;

	if (_rigidBody)
	{
		_rigidBody->setMassProps(_mass, btVector3());
	}
}

float RigidBodyComponent::GetRestitution() const
{
	return _restitution;
}

void RigidBodyComponent::SetRestitution(float restitution)
{
	_restitution = restitution;

	if (_rigidBody)
	{
		_rigidBody->setRestitution(restitution);
	}
}

void RigidBodyComponent::SetSize(float x, float y, float z)
{
	SetSize(float3(x, y, z));
}

void RigidBodyComponent::SetSize(const float3& colliderSize)
{
	_colliderConfig = colliderSize;
}

float3 RigidBodyComponent::GetGravity() const
{
	return _gravity;
}

void RigidBodyComponent::SetGravity(const float3& gravity)
{
	_gravity = gravity;

	if (_rigidBody)
		_rigidBody->setGravity(btVector3(_gravity.x, _gravity.y, _gravity.z));
}

void RigidBodyComponent::SetKinematic(bool kinematic)
{
	if (_rigidBody)
	{
		if (kinematic)
		{
			_rigidBody->setCollisionFlags(_rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}
		else if (_rigidBody->isKinematicObject())
		{
			_rigidBody->setCollisionFlags(_rigidBody->getCollisionFlags() ^ btCollisionObject::CF_KINEMATIC_OBJECT);
		}
	}

	_isKinematic = kinematic;
}

bool RigidBodyComponent::IsKinematic() const
{
	return _isKinematic;
}

float3 RigidBodyComponent::GetLinearFactor() const
{
	return _linearFactor;
}

void RigidBodyComponent::SetLinearFactor(const float3& factor)
{
	_linearFactor = factor;

	if (_rigidBody)
	{
		_rigidBody->setLinearFactor(btVector3(_linearFactor.x, _linearFactor.y, _linearFactor.z));
	}
}

float3 RigidBodyComponent::GetAngularFactor() const
{
	return _angularFactor;
}

void RigidBodyComponent::SetAngularFactor(const float3& factor)
{
	_angularFactor = factor;

	if (_rigidBody)
	{
		_rigidBody->setAngularFactor(btVector3(_angularFactor.x, _angularFactor.y, _angularFactor.z));
	}
}
