#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <numbers>

class PieChartShape : public sf::Drawable, public sf::Transformable
{
  public:
	explicit PieChartShape(float radius = 20.f) : m_radius(radius)
	{
		updateVertices();
	}

	void setProgress(float progress)
	{
		m_progress = std::clamp(progress, 0.f, 1.f);
		updateVertices();
	}

	void setRadius(float radius)
	{
		m_radius = radius;
		updateVertices();
	}

	void setFillColor(sf::Color const &color)
	{
		m_fillColor = color;
		updateVertices();
	}

	void setClockwise(bool clockwise)
	{
		m_clockwise = clockwise;
		updateVertices();
	}

	[[nodiscard]] float getProgress() const
	{
		return m_progress;
	}

	[[nodiscard]] float getRadius() const
	{
		return m_radius;
	}

  private:
	void draw(sf::RenderTarget &target, sf::RenderStates states) const override
	{
		states.transform *= getTransform();
		target.draw(m_vertices, states);
	}

	void updateVertices()
	{
		if(m_progress <= 0.f)
		{
			m_vertices.resize(0);
			return;
		}

		constexpr int SEGMENTS = 32;
		int numArcPoints = static_cast<int>(m_progress * SEGMENTS) + 1;
		int totalPoints = numArcPoints + 1; // +1 for center

		m_vertices.resize(totalPoints);
		m_vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);

		// Center point
		m_vertices[0].position = {0.f, 0.f};
		m_vertices[0].color = m_fillColor;

		float startAngle = -std::numbers::pi_v<float> / 2.f; // 12 o'clock position
		float sweepAngle = (m_clockwise ? 1.f : -1.f) * m_progress * 2.f * std::numbers::pi_v<float>;

		for(int i = 0; i < numArcPoints; ++i)
		{
			float t = static_cast<float>(i) / std::max(1, numArcPoints - 1);
			float angle = startAngle + t * sweepAngle;
			m_vertices[i + 1].position = {m_radius * std::cos(angle), m_radius * std::sin(angle)};
			m_vertices[i + 1].color = m_fillColor;
		}
	}

	sf::VertexArray m_vertices{sf::PrimitiveType::TriangleFan};
	sf::Color m_fillColor{255, 255, 255, 180};
	float m_radius = 20.f;
	float m_progress = 0.f;
	bool m_clockwise = true;
};
