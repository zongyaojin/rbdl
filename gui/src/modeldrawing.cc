#include "Model.h"
#include "Joint.h"
#include "Body.h"

#include <sstream>

#include <GL/gl.h>
#include <QDebug>

#include "glprimitives.h"

Vector3d compute_body_dimensions (Model* model, unsigned int body_id) {
	int j;

	// draw the body as a green box that extends from the origin to the
	// next joint
	Vector3d body_min (0., 0., 0.);
	Vector3d body_max (0., 0., 0.);

	if (model->mu.at(body_id).size() == 0) {
		// if there is no child we assume the body to extend to the COM.
		for (j = 0; j < 3; j++) {
			body_min[j] = std::min(body_min[j], model->mBodies[body_id].mCenterOfMass[j]);	
			body_max[j] = std::max(body_max[j], model->mBodies[body_id].mCenterOfMass[j]);	
		}
	}
	else {
		// if there are one or more children, we compute the body
		// dimensions such that the boundaries reach to the joint origins
		for (j = 0; j < model->mu.at(body_id).size(); j++) {
			unsigned int child_id = model->mu[body_id][j];
			Vector3d child_translation = model->X_T[child_id].get_translation();
			body_min[0] = std::min (body_min[0], child_translation[0]);
			body_max[0] = std::max (body_max[0], child_translation[0]);

			body_min[1] = std::min (body_min[1], child_translation[1]);
			body_max[1] = std::max (body_max[1], child_translation[1]);

			body_min[2] = std::min (body_min[2], child_translation[2]);
			body_max[2] = std::max (body_max[2], child_translation[2]);
		}
	}

	Vector3d body_dimensions = body_max - body_min;
	for (j = 0; j < 3; j++) {
		assert (body_dimensions[j] >= 0.);
		if (body_dimensions[j] == 0.) {
			body_min[j] = -0.1;
			body_max[j] = 0.1;
			body_dimensions[j] = 0.2;
		}
	}

	return body_dimensions;
}

void draw_model (Model* model) {
	glLineWidth (1.);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	glBegin (GL_LINES);
	glColor3f (1., 0., 0.);
	glVertex3f (0., 0., 0.);
	glVertex3f (1., 0., 0.);
	glColor3f (0., 1., 0.);
	glVertex3f (0., 0., 0.);
	glVertex3f (0., 1., 0.);
	glColor3f (0., 0., 1.);
	glVertex3f (0., 0., 0.);
	glVertex3f (0., 0., 1.);
	glEnd();

	glColor3f (1., 1., 1.);
	glDisable(GL_COLOR_MATERIAL);
	
	unsigned int i;
	for (i = 1; i < model->q.size(); i++) {
		// Draw only bodies with masses
		if (model->mBodies[i].mMass != 0) {
			Matrix3d rotation = model->GetBodyWorldOrientation(i);
			Vector3d translation = model->GetBodyOrigin(i);
			
			std::ostringstream model_X;
			model_X << model->X_base[i];

	//		qDebug() << "body " << i << ": " << model_X.str().c_str();
	//		qDebug() << "i = " << i << " translation = " << translation[0] << translation[1] << translation[2];	

			glPushMatrix();

			GLfloat orientation[16];
			int j,k;
			for (j = 0; j < 4; j++) {
				for (k = 0; k < 4; k++) {
					if (j < 3 && k < 3)
						orientation[j * 4 + k] = rotation(j,k);
					else 
						orientation[j * 4 + k] = 0.;
				} 
			}
			orientation[3 * 4 + 3] = 1.;

			// draw an orientation system of the current body
			glTranslated (translation[0], translation[1], translation[2]);
			glMultMatrixf(orientation);

			glBegin (GL_LINES);
			glColor3f (0.8, 0.8, 0.8);
			glVertex3f (0., 0., 0.);
			glVertex3f (1., 0., 0.);
			glVertex3f (0., 0., 0.);
			glVertex3f (0., 1., 0.);
			glVertex3f (0., 0., 0.);
			glVertex3f (0., 0., 1.);
			glEnd();

			//! \todo the body box is not well centered!
			// Draw the body
			Vector3d body_dimensions = compute_body_dimensions (model, i);

			glPushMatrix();
				glTranslated (
						body_dimensions[0] * 0.5,
						body_dimensions[1] * 0.5,
						body_dimensions[2] * 0.5
						);
				glScalef (
						body_dimensions[0] * 0.5,
						body_dimensions[1] * 0.5,
						body_dimensions[2] * 0.5
						);

				glEnable(GL_COLOR_MATERIAL);
				glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
				glColor3f (0., 0.9, 0.1);
				glprimitives_cube();
				glColor3f (1.0f, 1.0f, 1.0f);
				glDisable(GL_COLOR_MATERIAL);

			glPopMatrix();

			// draw the COM as a small cube of red color (we ignore the depth
			// buffer for better visibility 
			glTranslated (
					model->mBodies[i].mCenterOfMass[0],
					model->mBodies[i].mCenterOfMass[1],
					model->mBodies[i].mCenterOfMass[2]
					); 

			glDisable (GL_DEPTH_TEST);
			glEnable(GL_COLOR_MATERIAL);
			glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
			glColor3f (0.9, 0., 0.1);
			glScalef (0.025, 0.025, 0.025);
			glprimitives_cube();
			glColor3f (1.0f, 1.0f, 1.0f);
			glDisable(GL_COLOR_MATERIAL);
			glEnable (GL_DEPTH_TEST);

			glPopMatrix();
		}
	}
}