# Prototipo Simulador de Broncoscopía

**Autores:** Daniel Cíntora Morales, Daniel Octavio Delgado Vargas (Trabajo Terminal para obtenção do título de Ingeniero en Biónica)
**Orientadores:** M. en C. Rafael Santiago Godoy; M. en C. José Gonzalo Solís Villela
**Instituição:** Instituto Politécnico Nacional (IPN), Unidad Profesional Interdisciplinaria en Ingeniería y Tecnologías Avanzadas (UPIITA), México D.F.
**Fonte:** Repositório institucional do IPN (tesis.ipn.mx)
**URL:** https://tesis.ipn.mx/jspui/bitstream/123456789/19814/1/Prototipo%20simulador%20de%20broncoscop%C3%ADa.pdf
**PDF descarregado:** `IPN_prototipo_simulador_broncoscopia.pdf` (confirmado como PDF válido, assinatura `%PDF-1.6`)
**Acedido em:** 2026-07-06

## Resumo

Trabalho de fim de curso (2012) que desenvolve um "sistema háptico-virtual" para treino de broncoscopia. O sistema tem duas partes:

1. **Entorno virtual**: um modelo 3D das vias aéreas (traqueia, carina, brônquios principais/lobares) modelado em Rhinoceros e depois em Autodesk Maya, **exportado em formato wavefront (.obj) e renderizado em OpenGL/GLUT no ecrã do computador**. A câmara virtual desloca-se ao longo de trajetórias pré-definidas (splines amostradas) dentro do modelo, e a "colisão" é detetada por comparação de coordenadas, não por física real.
2. **Interface háptica física**: um "dummy" de broncoscopio (um cabo coaxial de 4 mm inserido através de um tubo guia de cobre) que passa por um mouse óptico (usado como *encoder* de posição/profundidade) e por uma pinça acionada por um único servomotor Dynamixel AX-12A. Essa pinça aperta ou solta o cabo do dummy para gerar sensações de fricção/colisão na mão do utilizador — **não move nem atua a anatomia impressa**, apenas restringe o movimento do instrumento simulado.
3. A "cara" externa é uma máscara estática de silicone/borracha (caucho de silicón) pintada, sem qualquer geometria de vias aéreas impressa em 3D nem qualquer atuação robótica própria.

Não existe impressão 3D da árvore brônquica como peça física manipulada — o "brônquio" só existe como modelo poligonal no ecrã. Não há qualquer menção a simulação de batimento cardíaco ou tosse; as sugestões de trabalho futuro (Capítulo 6) mencionam explicitamente que seria interessante, em trabalhos futuros, "incorporar animações prefijadas para simular los movimientos de respiración, de un paciente" — confirmando que a respiração **não estava implementada** nesta versão, e que mesmo essa sugestão futura seria apenas uma animação gráfica no ecrã, não um movimento mecânico da anatomia física.

## Relevância para a questão de originalidade

**Não é o que procuramos.** Este projeto não cumpre nenhuma das três características-chave:

- (a) Árvore brônquica impressa em 3D como peça física manipulada — **não**; a anatomia é só um modelo virtual 3D em OpenGL.
- (b) Atuação mecânica/robótica própria do modelo anatómico — **não**; o único atuador (servomotor Dynamixel) move uma pinça que aperta/solta o instrumento simulado (o "broncoscópio" dummy), não a anatomia.
- (c) Reprodução de respiração/batimento cardíaco/tosse — **não implementado**; é apenas sugerido como trabalho futuro sob a forma de animação gráfica no ecrã, não movimento físico.

Este é, portanto, um simulador háptico-virtual clássico (semelhante em conceito ao SensAble Phantom Omni mencionado no próprio "estado del arte" do documento), tecnologicamente distinto do conceito do projeto do estudante (manipuladores delta que atuam mecanicamente um modelo físico impresso em 3D para reproduzir respiração, batimento cardíaco e tosse). Reforça a tese de originalidade por contraste: mesmo um projeto de simulação de broncoscopia com "interfaz háptica" e "actuadores" usa esses atuadores apenas para dar resistência tátil à ferramenta, nunca para mover a própria anatomia impressa.
