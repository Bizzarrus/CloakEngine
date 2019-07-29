#ifndef PBR_HLSLI
#define PBR_HLSLI

#define PBR_FAST_VERSION
//#define PBR_NO_BRANCHING
#define PBR_IMPROVED_SIMPLE
#define PBR_OPTIMIZED_FRESNELL

#define PBR_MIN_ROUGHNESS 0.01f
#define PBR_LIQUID_ROUGHNESS PBR_MIN_ROUGHNESS

namespace PBR {
#define PI 3.14159265359f
#define RPI 0.31830988618f

	/*
	PBR Formulars :

	Fresnel Schlick approximation :
	F_Schlick(e, h) = c_spec + (1 - c_spec) * ((1 - dot(e, h)) ^ 5) - 2 * n * dot(e, h) * ((1 - dot(e, h)) ^ 6)
	with	h = halfway vector (v + l) / 2
	v = View direction
	l = Light direction
	e = either v or l

	Specular Color(offline computation) :
	c_spec = ((n - 1) ^ 2) / (((n + 1) ^ 2) + (k ^ 2))
	with	n = Index of Refraction
	k = Extinction coefficient

	Microfacet BRDF :
	f(l, v) = (F(l, h) * G(l, v, h) * D(h)) / (4 * dot(l, n) * dot(n, v))
	with	l = Light Direction
	h = Half way vector
	n = Surface normal
	v = View direction
	F = Fresnell approximation(per color value)
	G = Geometry Factor(scalar value)
	D = Normal Distribution(scalar value)
	See http://blog.selfshadow.com/publications/s2015-shading-course/hoffman/s2015_pbs_physics_math_slides.pdf for G and D functions
	See also https://mediatech.aalto.fi/~jaakko/T111-5310/K2013/03_RenderingEquation.pdf
	and http://graphicrants.blogspot.de/2013/08/specular-brdf-reference.html

	Lambert diffuse :
	f = (1 - m) * c_albedo / pi
	with	m = Metallic factor[0 - 1]

	Result = ((k_s * f_BRDF) + (k_d * f_Diffuse)) * c_light * dot(l, n)
	with	k_s = F(see Microfacet BRDF)
	k_d = (1 - k_s)

	See https://learnopengl.com/#!PBR/Theory

	Wet surfaces: https://seblagarde.wordpress.com/2013/03/19/water-drop-3a-physically-based-wet-surfaces/

	*/

	struct LightResult {
		float4 Diffuse;
		float4 Specular;
	};

	//To get Fresnell Value: float3 x = F_Schlick(d); Fresnell = x.x + (specular * x.y) + (ior * x.z)
	float2 F_Schlick(float d)
	{
#ifdef PBR_OPTIMIZED_FRESNELL
		//Optimization based on https://seblagarde.wordpress.com/2011/08/17/hello-world/
		float f5 = exp2(-8.656170245f * d); // 8.656170245 = 6 / ln(2)
#else
		float f1 = 1 - d;
		float f2 = f1 * f1;
		float f5 = f2 * f2*f1;
#endif
		return float2(f5, 1 - f5);
	}
	float D_GGX(float r2, float dotNH)
	{
		float r4 = r2 * r2;
		float dn = (dotNH*dotNH*(r4 - 1)) + 1;
		return r4 * rcp(dn*dn);
	}
	float G_SchlickGGX(float r2, float dotVN)
	{
		float k = 0.5f*r2;
		float d = lerp(dotVN, 1, k);
		return dotVN * rcp(d);
	}
	float V_SchlickGGX(float r2, float dotVH)
	{
		float k = 0.5f*r2;
		float d = lerp(dotVH, 1, k);
		return rcp(d*d);
	}
	float V_SchlickGGX(float r2, float dotNV, float dotNL)
	{
		float k = 0.5f*r2;
		float d1 = lerp(dotNV, 1, k);
		float d2 = lerp(dotNL, 1, k);
		return rcp(d1*d2);
	}

	LightResult Light(float3 viewDir, float3 lightDir, float3 normalDir, float3 albedo, float3 specular, float4 light, float roughness, float ao, float specLiquid, float porosity, float wetLevel, float liquidRefract)
	{
		LightResult res;
		res.Diffuse = float4(0, 0, 0, 0);
		res.Specular = float4(0, 0, 0, 0);
#ifdef PBR_NO_BRANCHING
		float dotNL = saturate(dot(lightDir, normalDir));
#else
		float dotNL = dot(lightDir, normalDir);
		if (dotNL > 0)
		{
#endif

#ifdef PBR_IMPROVED_COMPLEX
			float3 H1 = normalize(viewDir + lightDir);
			float dotNH1 = saturate(dot(normalDir, H1));
			float dotVH1 = saturate(dot(viewDir, H1));
			float3 VD2 = lerp(viewDir, -refract(-viewDir, normalDir, liquidRefract), wetLevel);
			float3 LD2 = lerp(lightDir, -refract(-lightDir, normalDir, liquidRefract), wetLevel);
			float3 HD2 = normalize(VD2 + LD2);
			float dotNH2 = saturate(dot(normalDir, HD2));
			float dotVH2 = saturate(dot(VD2, HD2));
			float2 F1 = F_Schlick(dotVH1);
			float2 F2 = F_Schlick(dotVH2);
			float F1_in = F1.x + (F1.y * specLiquid);
			float F1_out = F1_in;
			float3 F2_in = F2.x + (F2.y * specular);
			float3 F2_out = F2_in;

#elif defined(PBR_COMPLEX)
			float3 H1 = normalize(viewDir + lightDir);
			float dotNH1 = saturate(dot(normalDir, H1));
			float dotVH1 = saturate(dot(viewDir, H1));
			float dotNV1 = saturate(dot(normalDir, viewDir));
			float3 VD2 = lerp(viewDir, -refract(-viewDir, normalDir, liquidRefract), wetLevel);
			float3 LD2 = lerp(lightDir, -refract(-lightDir, normalDir, liquidRefract), wetLevel);
			float3 HD2 = normalize(VD2 + LD2);
			float dotNH2 = saturate(dot(normalDir, HD2));
			float dotVH2 = saturate(dot(VD2, HD2));
			float dotNV2 = saturate(dot(normalDir, VD2));
			float dotNL2 = saturate(dot(normalDir, LD2));
			float2 F_in1 = F_Schlick(dotNL);
			float2 F_out1 = F_Schlick(dotNV1);
			float2 F_in2 = F_Schlick(dotNL2);
			float2 F_out2 = F_Schlick(dotNV2);
			float F1_in = F_in1.x + (F_in1.y * specLiquid);
			float F1_out = F_out1.x + (F_out1.y * specLiquid);
			float3 F2_in = F_in2.x + (F_in2.y * specular);
			float3 F2_out = F_out2.x + (F_out2.y * specular);

#elif defined(PBR_IMPROVED_SIMPLE)
			float3 halfway = normalize(viewDir + lightDir);
			float dotNH1 = saturate(dot(normalDir, halfway));
			float dotNH2 = dotNH1;
			float dotVH1 = saturate(dot(viewDir, halfway));
			float dotVH2 = dotVH1;
			float2 F = F_Schlick(dotVH1);
			float F1_in = F.x + (F.y * specLiquid);
			float F1_out = F1_in;
			float3 F2_in = F.x + (F.y * specular);
			float3 F2_out = F2_in;
#else
			float3 halfway = normalize(viewDir + lightDir);
			float dotNH1 = saturate(dot(normalDir, halfway));
			float dotNH2 = dotNH1;
			float dotVH1 = saturate(dot(viewDir, halfway));
			float dotVH2 = dotVH1;
			float dotNV = saturate(dot(viewDir, normalDir));
			float2 F_in = F_Schlick(dotNL);
			float2 F_out = F_Schlick(dotNV);
			float F1_in = F_in.x + (F_in.y * specLiquid);
			float F1_out = F_out.x + (F_out.y * specLiquid);
			float3 F2_in = F_in.x + (F_in.y * specular);
			float3 F2_out = F_out.x + (F_out.y * specular);
#endif

			//Remapping Roughness from 0..1 to PBR_MIN_ROUGHNESS..1
			float r = lerp(roughness, 1, PBR_MIN_ROUGHNESS);
#ifdef PBR_FAST_VERSION
			float V1 = V_SchlickGGX(r, dotVH1);
			float V2 = V_SchlickGGX(PBR_LIQUID_ROUGHNESS, dotVH2);
			float G = V1 * dotVH1 * dotVH1;
#else
			float V1 = V_SchlickGGX(r, dotNV, dotNL);
			float V2 = V_SchlickGGX(PBR_LIQUID_ROUGHNESS, dotNV, dotNL);
			float G = V1 * dotNV * dotNL;
#endif
			float D1 = D_GGX(r, dotNH1);
			float D2 = D_GGX(PBR_LIQUID_ROUGHNESS, dotNH2);

			float3 BRDF_D = albedo;
			float3 BRDF_S = F2_out * V1 * D1 * 0.25f;
			float BRDF_L = F1_out * V2 * D2 * 0.25f;

			/*
			Calculation of Wet Surface BRDF:
			- f = (F_liquid_in * wetLevel * (1 - porosity) * BRDF_L) // Double Layer BRDF liquid specular
				+ (lerp(1, (1 - F_liquid_in) * (1 - F_liquid_out) * A_l, wetLevel * (1 - porosity)) //Double Layer BRDF liquid diffuse
				* f_porosity) // Material specular & diffuse
			- f_porosity = (F_material_in * (1 - porosity * G) * BRDF_S) //Porosity material specular
				+ ((1 - F_material_in) * (1 - porosity * G) * BRDF_D) //Porosity material diffuse (non porose part)
				+ ((1 - wetLevel) * porosity * G * A_p * BRDF_D) //Porosity dry
				+ (wetLevel * porosity * G * A_p * BRDF_L) //Porosity wet
			- A_p = (F_material_in + (1 - F_material_in)) ^ (nb + 1) = 1 //Absorption by surface bounces
			- A_l = 1 - thickness * absorptionFactor ~ 1 //Absorption by water layer, roughly one (no absorption) as layer is very thin (thickness ~ 0)

			=> 	f = Kl * BRDF_L + Kd * f_porosity
					Kl = F_liquid_in * wetLevel * (1 - porosity)
					Kd = lerp(1, (1 - F_liquid_in) * (1 - F_liquid_out) * A_l, wetLevel * (1 - porosity))
				f_porosity = Ksp' * BRDF_S + Kdp' * BRDF_D + Klp * BRDF_L
					Ksp' = F_material_in * (1 - porosity * G)
					Kdp' = ((1 - F_material_in) * (1 - porosity * G)) + ((1 - wetLevel) * porosity * G * A_p) = lerp(1 - F_material_in, 1 - wetLevel, porosity * G)
					Klp = wetLevel * porosity * G * A_p

			=>	Ksl = Kl + Kd * Klp = F_liquid_in * wetLevel * lerp(1, G * Kd, porosity)
				Ksp = Kd * Ksp'
				Kdp = Kd * Kdp'

				f = Ksl * BRDF_L + Ksp * BRDF_S + Kdp * BRDF_D
			*/

			float aG = porosity * G;
			
			float Kp = lerp(1, (1 - F1_in) * (1 - F1_out), (1 - porosity) * wetLevel);
			float3 Ksp = Kp * F2_in * (1 - aG);
			float3 Kdp = Kp * lerp(1 - F2_in, 1 - wetLevel, aG);
			float3 Ksl = F1_in * wetLevel * lerp(1, G * Kp, porosity);

			float3 lp = light.xyz * light.w * dotNL * ao * RPI;
			res.Diffuse = float4(lp * Kdp * BRDF_D, 1);
			//TODO: Add transcluent (Sub Surface Scattering) to diffuse
			res.Specular = float4(lp * ((Ksp * BRDF_S) + (Ksl * BRDF_L)), 1);
#ifndef PBR_NO_BRANCHING
		}
#endif
		return res;
	}
	LightResult IBL(float3 albedo, float3 irradiance, float2 lut, float3 firstSum, float3 specular, float ao, float dotNV, float porosity, float wetLevel, float specLiquid, float roughness)
	{
		LightResult res;
		//Remapping Roughness from 0..1 to PBR_MIN_ROUGHNESS..1
		float r = lerp(roughness, 1, PBR_MIN_ROUGHNESS);
		float ir = 1 - r;

		//Scaling factor based on https://learnopengl.com/PBR/IBL/Diffuse-irradiance
		float F1_scale = max(ir, specLiquid);
		float3 F2_scale = max(ir, specular);

		float2 F_out = F_Schlick(dotNV);
		float F1_out = (F1_scale * F_out.x) + (F_out.y * specLiquid);
		float3 F2_out = (F2_scale * F_out.x) + (F_out.y * specular);

		float3 radiance = (firstSum * (F2_out * lut.x) + lut.y);

		float G1 = G_SchlickGGX(r, dotNV);
		float G2 = (((0.3586f*r) - 0.7449f)*r) + 1;//Approximation of G_SchlickGGX for dotNL in range 0..1, based on visual graph of actual integral
		float G = G1 * G2;
		float aG = porosity * G;

		float Kp = lerp(1, (1 - F1_out) * (1 - F1_out), (1 - porosity) * wetLevel);
		float3 Ksp = Kp * F2_out * (1 - aG);
		float3 Kdp = Kp * lerp(1 - F2_out, 1 - wetLevel, aG);
		float3 Ksl = F1_out * wetLevel * lerp(1, G * Kp, porosity);

		res.Diffuse = float4(irradiance * ao * Kdp * albedo * RPI * 0.25f, 1);
		res.Specular = float4(radiance * ao * (Ksp + Ksl), 1);
		return res;
	}
	float4 Ambient(float4 albedo, float3 specular, float4 light, float roughness, float ao, float specLiquid, float porosity, float wetLevel)
	{
		float c = albedo.w > 0 ? 1 : 0;
		float3 Kdp = lerp(1 - specular, lerp(1, specLiquid, wetLevel), porosity);

		float3 BRDF_D = albedo.xyz;

		float nsl = 1 - specLiquid;
		float np = (1 - porosity)*wetLevel;
		return c * float4(light.xyz*light.w*ao*RPI*((Kdp*BRDF_D)*(1 + (np*((nsl*nsl) - 1)))), 1);
	}
	float LightIntensity(float distance, float maxDistance)
	{
		/*
		float a = 6 / maxDistance;
		float b = 3 / (maxDistance*maxDistance);
		float d = (((b*distance) + a)*distance) + 3;
		return clamp((4 / d) - 1 / 3, 0, 1);
		*/
		/*
		float a = clamp(1 - (distance / maxDistance), 0, 1);
		return a*a;
		*/
		float d2 = distance * distance;
		float md2 = maxDistance * maxDistance;
		float d = d2 / md2;
		d = d * d;

		float nom = saturate(1 - d);
		return (nom*nom) / (d2 + 1);
	}
}

#endif