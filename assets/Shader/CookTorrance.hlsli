#ifndef __COOKTORRANCE_HLSLI__
#define __COOKTORRANCE_HLSLI__

float3 CookTorranceDiffuse(in Material mat, in float3 F, in float NoL) {
    const float3 Diffuse = mat.Albedo.rgb / PI;

    float3 kD = 1.f - F;
    kD *= (1.f - mat.Metalness);

    return kD * Diffuse * NoL;
}

float3 CookTorranceSpecular(
        in Material mat,
        in float3 L,
        in float3 N,
        in float3 V,
        in float NoL) {
    const float3 H = normalize(V + L);
    const float Roughness = mat.Roughness;

    const float NDF = DistributionGGX(N, H, Roughness);
    const float G = GeometrySmith(N, V, L, Roughness);
    const float3 F = FresnelSchlick(saturate(dot(H, V)), mat.FresnelR0);

    const float3 Numerator = NDF * G * F;
    const float Denominator = 4.f * max(dot(N, V), 0.f) * max(NoL, 0.f) + 0.0001f;

    return Numerator / Denominator;
}

float3 CookTorrance(
        in Material mat,
        in float3 Li,
        in float3 L,
        in float3 N,
        in float3 V,
        in float NoL) {
    const float3 H = normalize(V + L);
    const float3 F = FresnelSchlick(saturate(dot(H, V)), mat.FresnelR0);

    const float3 diffuse = CookTorranceDiffuse(mat, F, NoL);
    const float3 specular = CookTorranceSpecular(mat, L, N, V, NoL);

    return (diffuse + specular * NoL) * Li;
}

#endif // __COOKTORRANCE_HLSLI__