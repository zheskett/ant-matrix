export async function onRequest(ctx) {
    const resp = await ctx.next();
    resp.headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
    resp.headers.set('Cross-Origin-Opener-Policy', 'same-origin');
    return resp;
}